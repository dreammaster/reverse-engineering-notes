"""
IDA Pro script: master list of symbol renames (functions + globals) for
OUT.EXE (ultima1_out.idb).

Single accumulating script instead of one standalone name_*.py file per
finding, mirroring the ultima2 project's apply_renames.py convention.
Whenever a function's or global's role becomes clear enough to name
confidently, add an entry to RENAMES below and re-run. Safe to re-run
repeatedly -- each entry is checked against the address's *current* name
and skipped if already applied.

Convention: DRY_RUN starts True here (unlike ultima2's apply_renames.py,
which Paul flipped to False on 2026-08-17 after the pattern proved
reliable there -- no equivalent decision made for this project yet).
Run once with DRY_RUN True, check the output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName apply_renames_out.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x1A81D, "getKeypressAndWaitRaw",
     "poll-loop identical to getKeypressAndWait (getKeypress + wait(1) or "
     "wait(3) depending whether a key was ready) but WITHOUT the _toupper "
     "call -- getKeypressAndWait is a separate, independent copy of this "
     "same loop, not a wrapper around this. All 15 call sites push "
     "_textColor (never any other value); some callers _toupper the "
     "result themselves afterward, others don't. See "
     "docs/overview.md#getkeypressandwaitraw-and-getkeypressandwait-duplicated-poll-loop."),

    (0x1A30C, "toLowerLetter",
     "hand-rolled tolower: if arg is 'A'-'Z' add 20h, else pass through "
     "unchanged. Called right after getKeypressAndWaitRaw in several "
     "menu-selection paths (drop/dropArmor/dropWeapon/etc.) to normalize "
     "a letter keypress before subtracting 'a' to get a menu index."),

    (0x1981D, "_nmalloc",
     "near-heap allocator: walks a singly-linked free-list "
     "(word_1EC56=sentinel head, word_1EC5A=cursor), splits a free block "
     "if it's big enough, else calls _nheapgrow to extend the segment via "
     "DOS INT 21h/4Ah (SETBLOCK) and links the new space in via _nfree "
     "before retrying. Underscore-prefixed to match this codebase's "
     "existing convention for runtime-library internals (_fopen, "
     "_toupper) -- this is the classic Microsoft C near-heap allocator, "
     "not game-specific code. See "
     "docs/overview.md#near-heap-allocator-_nmalloc-_nfree-_nheapgrow."),

    (0x198C0, "_nfree",
     "near-heap free: walks the same free-list as _nmalloc, coalesces "
     "the freed block with adjacent free blocks on either side. See "
     "_nmalloc entry above."),

    (0x19EA3, "_nheapgrow",
     "grows the near heap via DOS INT 21h/4Ah (ADJUST MEMORY BLOCK "
     "SIZE). Only called from _nmalloc. Note: this proc's disassembly "
     "contains two additional 'push bp; mov bp,sp; ...' prologues after "
     "the first RETN with NO incoming xrefs (dead/unreachable) -- almost "
     "certainly two sibling CRT entry points (e.g. a realloc/msize "
     "variant) from the same library object file that got linked in but "
     "never called by this program. Left un-split since they're inert; "
     "worth revisiting if a future finding calls into the middle of this "
     "proc."),

    (0x1B0B5, "readAmount",
     "up-to-4-digit numeric text entry at a screen position: isDigit-"
     "filters keypresses (via getKeypressAndWaitRaw), handles backspace "
     "(char 8) to delete the last digit, on Enter (or any non-digit/"
     "non-backspace key) converts the accumulated digit buffer to a "
     "number using word_1F95E (a {1,10,100,...} powers-of-ten table -- "
     "currently mis-typed in the IDB as one `dw 1` word followed by raw "
     "db bytes, not a proper array; see roadmap.md). Callers: dropPence, "
     "transactCastle, transactGrocer -- all 'how many/how much' prompts."),

    (0x1B094, "isDigit",
     "byte in ['0'..'9'] check, returns 1/0 in ax. Only caller is "
     "readAmount."),

    # -- second pass: the CRT buffered-stdio + low-level DOS I/O layer
    # underneath readFile/writeFile/_fopen. Confirmed by reading the
    # bodies directly (fread/fwrite-shaped double loops, the classic
    # _filbuf/_flsbuf refill/flush interaction, and -- the clincher -- a
    # tight cluster of one-INT-21h-call wrappers that match Microsoft
    # C's documented <dos.h> primitives (_dos_open/_dos_close/_dos_read/
    # _dos_write/_dos_lseek/_dos_creat/_dos_creatnew/_dos_creattemp)
    # almost exactly, including which AH= subfunction each one uses. See
    # docs/overview.md#outexe-crt-file-io-layer-decoded. --

    (0x1C8F2, "_fread",
     "byte-at-a-time buffered read: outer loop bounded by arg_4, inner "
     "by arg_2, refills via _filbuf when the stream's buffer count goes "
     "negative. Called (buffer, count, 1, stream) from readFile -- note "
     "readFile's own stack frame mis-declares its buffer+count args as "
     "one dword 'buffer', an IDB bug worth fixing separately (see "
     "roadmap.md), not a real far pointer."),

    (0x1C960, "_fwrite",
     "write-side mirror of _fread: flushes via _flsbuf when the "
     "stream's write-buffer count is exhausted. Called from writeFile."),

    (0x1C6D4, "_fclose",
     "flushes a dirty write buffer via _flsbuf, frees the stream's "
     "buffer via _nfree if one was allocated, zeroes the buffer/flags "
     "fields, then releases the OS handle via releaseFileHandle. "
     "Returns 0 or -1 (0FFFFh)."),

    (0x1CC6B, "_filbuf",
     "refill-buffer-and-return-one-char: flushes a pending write buffer "
     "first if the stream is in mixed read/write mode, ensures a buffer "
     "exists (allocFileBuffer), reads more data via readLowLevel, and "
     "does text-mode translation (strips CR 0Dh, recognizes Ctrl-Z 1Ah "
     "as EOF) when the stream isn't opened binary."),

    (0x1CE05, "_flsbuf",
     "flush-buffer-and-write-one-char: mirror of _filbuf for the write "
     "side. Signature (int c, FILE *stream) -- confirmed by _fwrite's "
     "call site pushing (char, stream) in that order."),

    (0x1D30F, "allocFileBuffer",
     "lazily _nmalloc's a stream's I/O buffer (size from word_20EE0) on "
     "first use; shared by _filbuf and _flsbuf. Not a standard CRT name "
     "I could confidently match -- descriptive name instead."),

    (0x1C791, "_openfile",
     "the real body of _fopen (which is just a thin 'close any "
     "previously-open stream on this FILE*, then call this' wrapper): "
     "parses the mode string ('r'/'w'/'a', 'b', '+') into open flags "
     "and calls _open."),

    (0x1CA14, "_open",
     "composite open: interprets O_CREAT/O_TRUNC/O_EXCL-style flag bits "
     "(0x100/0x200/0x400, matching MSC's fcntl.h values exactly) and "
     "dispatches to whichever raw DOS primitive matches -- _dos_open, "
     "_dos_creat, _dos_creatnew, or _dos_creattemp -- then records the "
     "resulting handle. Called only from _openfile."),

    (0x1C9CC, "releaseFileHandle",
     "looks up the internal handle-table slot for a DOS handle "
     "(findFileHandleSlot), closes it via _dos_close, clears the slot. "
     "The layer _fclose calls; not itself a raw DOS wrapper."),

    (0x1D0DD, "findFileHandleSlot",
     "linear scan of a fixed-size internal table (word_1EC60 entries, "
     "4 words each at 17E2h+idx*4: in-use flag, DOS handle, ...) for "
     "the slot matching a given DOS handle. Sets errno=9 (EBADF) and "
     "returns 0 if not found."),

    (0x190C0, "_flushall",
     "iterates the 20-entry, 14-bytes-each static FILE table (base "
     "0B124h -- almost certainly the classic CRT _iob[] array, though "
     "not renamed here since it's referenced only as an arithmetic "
     "literal, not a labeled address IDA already recognizes) and "
     "flushes any stream with pending unwritten data via _dos_lseek + "
     "write. Called from readSavegame's invalid-save-slot fallback and "
     "from start2."),

    # -- raw one-INT-21h-call DOS primitives, all sharing the same "jmp "
    # to a common error handler on failure" shape. Names match MSC's
    # documented dos.h functions exactly, confirmed by AH= subfunction. --

    (0x19578, "_dos_open", "AH=3Dh OPEN DISK FILE WITH HANDLE."),
    (0x19589, "_dos_close", "AH=3Eh CLOSE A FILE WITH HANDLE."),
    (0x19597, "_dos_read", "AH=3Fh READ FROM FILE WITH HANDLE."),
    (0x195AB, "_dos_lseek",
     "AH=42h MOVE FILE READ/WRITE POINTER; returns the new 32-bit "
     "position packed in bx:ax rather than via an out-parameter, unlike "
     "the textbook _dos_lseek signature -- same DOS call underneath, "
     "the C-level calling convention just differs here."),
    (0x195CF, "_dos_write",
     "AH=40h WRITE TO FILE WITH HANDLE; sets errno=28 (ENOSPC) if "
     "fewer bytes were written than requested (disk full)."),
    (0x195F8, "_dos_ioctl_get",
     "AX=4400h IOCTL GET DEVICE INFORMATION. No single documented "
     "dos.h name fits this subfunction exactly (it's what isatty()-"
     "style checks are built on); descriptive name instead."),
    (0x1D36A, "_dos_ioctl_set", "AX=4401h IOCTL SET DEVICE INFORMATION."),
    (0x1D37C, "_dos_creat", "AH=3Ch CREATE A FILE WITH HANDLE (CREAT)."),
    (0x1D38D, "_dos_creatnew",
     "AH=5Bh CREATE NEW FILE (fails if the file already exists -- the "
     "O_CREAT|O_EXCL case)."),
    (0x1D39A, "_dos_creattemp", "AH=5Ah CREATE UNIQUE FILE."),

    # -- third pass: the shop-transaction cluster underneath
    # transactWeapons/transactArmory/transactMagic/transactTransport.
    # Each shop has its own buy-price formula, and weapons/armor also
    # have a sell-price formula and a dedicated sell flow (magic can't
    # be sold -- transactMagic prints "Sorry, we don't buy spells!" on
    # the 'S' branch instead of calling a sell helper, which is why
    # there's no sellMagic here). See
    # docs/overview.md#outexe-shop-transaction-cluster-decoded. --

    (0x163B3, "calcWeaponBuyPrice",
     "(255 - _savegame._intelligence) * weaponIndex^2 / 256 + 5. "
     "Quadratic in weaponIndex -- higher-tier weapons cost "
     "disproportionately more. Called from drawWeaponShopLine and "
     "transactWeapons' purchase-confirm step."),

    (0x163D4, "calcWeaponSellPrice",
     "(_savegame._charisma + 40) * weaponIndex^2 / 256 + 1 -- sell "
     "price uses charisma instead of intelligence, and a different "
     "additive offset, but the same quadratic shape as "
     "calcWeaponBuyPrice. Called from sellWeapons."),

    (0x165CE, "drawWeaponShopLine",
     "draws one 'a) Weapon Name    - NNN' line in the weapons shop's "
     "buy list: letter, name (WEAPONS_UPPERCASE[]), price via "
     "calcWeaponBuyPrice. Called from transactWeapons' buy-loop."),

    (0x163F2, "sellWeapons",
     "the 'S' branch of transactWeapons: scans _savegame._weapons_array "
     "for owned weapons, shows 'Thou hast no weaponry to sell!' if none, "
     "else lists them with sell prices (calcWeaponSellPrice) and "
     "processes the chosen sale. Signature confirmed by transactWeapons "
     "comparing buySell against 83 ('S') before calling this."),

    (0x167F1, "calcArmorBuyPrice",
     "(200 - _savegame._intelligence) / 4 * armorIndex -- linear in "
     "armorIndex, unlike the weapon shop's quadratic formula."),

    (0x1680C, "calcArmorSellPrice",
     "(_savegame._charisma / 4) * armorIndex."),

    (0x169E8, "drawArmorShopLine",
     "armor-shop equivalent of drawWeaponShopLine, using "
     "calcArmorBuyPrice. transactArmory calls it for indices 1-3 "
     "always, 4-5 only once _moveCtr passes 3000 (same time-gated "
     "unlock pattern as the weapons shop)."),

    (0x16823, "sellArmor",
     "armor-shop equivalent of sellWeapons, using calcArmorSellPrice. "
     "Called from transactArmory's 'S' branch."),

    (0x16BAC, "calcMagicBuyPrice",
     "(200 - _savegame._wisdom) / 32 * spellIndex -- linear, third stat "
     "(wisdom) used for the third shop type."),

    (0x16BC7, "drawMagicShopLine",
     "magic-shop equivalent of drawWeaponShopLine, using "
     "calcMagicBuyPrice. No sell counterpart exists -- transactMagic's "
     "'S' branch prints 'Sorry, we don't buy spells!' instead of "
     "calling a sell helper."),

    (0x17052, "drawTransportShopLine",
     "transport-shop equivalent of drawWeaponShopLine, but reuses the "
     "already-named getExpense for its price instead of a dedicated "
     "calc*Price helper -- transport pricing was evidently handled "
     "elsewhere/earlier than this cluster. Signature comment was "
     "already present: void __cdecl(int transportId, int yp, char c)."),

    (0x192D4, "divmod32",
     "full signed 32-bit division: quotient in ax:bx, remainder in "
     "cx:dx, both sign-corrected at the end -- the classic shift-"
     "subtract long-division algorithm (16 and 32 iteration loops for "
     "the two magnitude cases), almost certainly a compiler-emitted "
     "arithmetic helper rather than game logic (8086 has no native "
     "32-bit divide). Named for its confirmed mechanical operation only "
     "-- NOT confident enough in the exact original toolchain symbol "
     "(e.g. Microsoft's _alldiv/_aNdiv naming varies by compiler "
     "version) to use one. Its role in transactWeapons -- chaining "
     "divmod32(_moveCtr, 0:0x7FFFh) then divmod32(<that remainder>, "
     "0:1500) to help pick var_A -- looks like a playtime-gated "
     "weapon-tier-unlock calculation, but the exact formula's meaning "
     "isn't pieced together yet; flagged as a follow-up in "
     "roadmap.md rather than asserted here."),

    # -- fourth pass: the `get` command's castle item-theft mechanic. --

    (0x15469, "denyGetNoPermission",
     "prints \"Thou hast not the king's permission!\" plus a failure "
     "sound (playFX(1)). Called when _castleItemAllowance is exhausted."),

    (0x15492, "getArmorTile",
     "if _castleItemAllowance > 0, decrements it and calls findArmor; "
     "else denyGetNoPermission. One of 3 near-identical wrappers "
     "(getWeaponTile, getFoodTile) dispatched by getTownItem based on "
     "the map tile under the player."),

    (0x154B5, "getWeaponTile", "weapon counterpart of getArmorTile."),

    (0x154D8, "getFoodTile", "food counterpart of getArmorTile."),

    (0x154FB, "getTownItem",
     "the `get` command's handler for town/castle maps: reads the "
     "_townCityMap tile at the player's position and dispatches to "
     "getFoodTile ('9'), getArmorTile ('7'), or getWeaponTile (';'), "
     "else prints ' - nothing here!'. In practice these tile codes seem "
     "to appear only in castle room layouts (matching the "
     "'king's permission' framing), not ordinary town tiles."),

    (0x15B8A, "checkCaughtStealing",
     "guard-detection roll shared by findArmor/findWeapon/findFood "
     "(all three already named): rolls 1-255, catches the player if "
     "the roll is low (<0x26) or guards are already hostile -- unless "
     "the player's class is Thief (_savegame._class==3), which always "
     "succeeds silently. On a catch, prints \"Oh no!  Thou wert "
     "caught!\" and sets _guardsHostile."),
]

# (ea, new_name, note) -- globals in the same CRT file-I/O cluster,
# identified by the exact numeric values stored into them matching
# well-known MSC errno.h / dos.h semantics.
GLOBAL_RENAMES = [
    (0x1D4F1, "_doserrno",
     "set by _nheapgrow and the buffered-I/O layer after any DOS call; "
     "readFile/writeFile check it after _fopen/_fread/_fwrite/_fclose "
     "to decide whether to prompt insertDisk and retry. Paired with "
     "errno below, matching MSC's dual raw-DOS-code/translated-POSIX-"
     "code error convention."),
    (0x1ED22, "errno",
     "confirmed by the exact values stored: 9 (EBADF) when "
     "findFileHandleSlot can't find a handle, 0x11/17 (EEXIST) and "
     "0x16/22 (EINVAL) in _open's O_CREAT/O_EXCL branches, 0x18/24 "
     "(EMFILE) when _fopen's FILE table is full, 0x1C/28 (ENOSPC) on a "
     "short _dos_write. All match MSC errno.h exactly."),
    (0x1EC50, "_fmode",
     "read by _openfile as the starting text/binary translation flag "
     "before the mode string's 'b'/'a' override it -- matches MSC's "
     "global default-file-mode variable."),

    (0x26024, "_castleItemAllowance",
     "set to 9 on entering a castle (in enterCastle) and to 0 on "
     "entering other location types; decremented once per successful "
     "getArmorTile/getWeaponTile/getFoodTile call, and once it hits 0 "
     "further `get` attempts print \"Thou hast not the king's "
     "permission!\" via denyGetNoPermission."),
]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    for ea, new_name, note in GLOBAL_RENAMES:
        apply_rename(ea, new_name, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()

"""
IDA Pro script: master list of symbol renames (functions + globals) for
GEN.EXE (ultima1_gen.idb).

Single accumulating script per the convention established in
apply_renames_out.py/apply_renames_ultima1.py. Whenever a finding is
confirmed, add an entry to RENAMES/GLOBAL_RENAMES below and re-run.

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_gen -ScriptName apply_renames_gen.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- first pass: GEN.EXE links the exact same Microsoft C runtime
    # object files as OUT.EXE/ULTIMA.EXE -- confirmed by direct reads
    # (mode-string parsing, free-handle-table walk shape) plus several
    # exact byte-size matches to OUT.EXE's already-decoded functions.
    # See docs/overview.md#genexe-crt-layer-and-exec-chain-to-outexe. --

    (0x12491, "_flsbuf",
     "flush-buffer-and-write-one-char, matching OUT.EXE's _flsbuf "
     "exactly in size (728 bytes)."),
    (0x127E8, "findFileHandleSlot",
     "internal handle-table lookup, matching OUT.EXE's "
     "findFileHandleSlot exactly in size (73 bytes)."),
    (0x120A0, "_open",
     "composite open: walks a free-handle-table-slot search identical "
     "in shape to OUT.EXE's _open, confirmed by direct read. 562 "
     "bytes -- exact match to OUT.EXE's _open."),
    (0x122F7, "_filbuf",
     "refill-buffer-and-return-one-char, matching OUT.EXE's _filbuf "
     "exactly in size (410 bytes)."),
    (0x128AA, "_read",
     "text-mode-aware read underneath _filbuf, matching OUT.EXE's "
     "_read exactly in size (199 bytes)."),
    (0x12DEA, "_lseek",
     "CRT-level seek used by _read/_write, matching OUT.EXE's _lseek "
     "exactly in size (213 bytes)."),
    (0x12C04, "allocFileBuffer",
     "lazily allocates a stream's I/O buffer, matching OUT.EXE's "
     "allocFileBuffer exactly in size (91 bytes)."),
    (0x11A3A, "_openfile",
     "the real body of fopen: parses the mode string, called from "
     "accessSavegame (already named). 353 bytes -- exact match to "
     "OUT.EXE's _openfile."),

    # -- exec-chain cluster: GEN.EXE launches OUT.EXE (base filename "
    # "\"out\", extension appended by findExecutableFile) passing the "
    # "video-mode letter ('C'/'E'/'T') and the save slot number as "
    # "argv[1]/argv[2] -- confirmed by reading launchGame directly, "
    # "which also prints \"Please wait whilst thy game loads\" right "
    # "before the chain call. Same no-DOS-EXEC overlay-loader "
    # "mechanism decoded in OUT.EXE/ULTIMA.EXE. --

    (0x11948, "chainToExecutable",
     "pushes (filename, &argv, envp) and calls execProgramEntry, "
     "matching OUT.EXE/ULTIMA.EXE's chainToExecutable exactly."),
    (0x11F4E, "execProgramEntry",
     "findExecutableFile then buildAndChainExecutable, matching "
     "OUT.EXE/ULTIMA.EXE exactly."),
    (0x12831, "findExecutableFile",
     "locates the target program file by trying the bare name then "
     "appending default extensions."),
    (0x12A93, "buildAndChainExecutable",
     "builds the DOS command-tail buffer from argv then calls "
     "execProgram."),
    (0x13143, "execProgram",
     "the actual overlay loader -- 555 bytes, exactly matching "
     "OUT.EXE/ULTIMA.EXE's execProgram in size."),
    (0x11643, "launchGame",
     "called from continuePreviousGame: prints 'Please wait whilst "
     "thy game loads', then loops calling chainToExecutable with "
     "filename \"out\" (extension added by findExecutableFile) and "
     "argv[1]/argv[2] set to a video-mode letter ('C' default, "
     "patched to 'E' for EGA or 'T' for Tandy based on _videoMode) "
     "and a save-slot digit ('0'+slot) -- i.e. GEN.EXE launches OUT.EXE "
     "as roughly 'OUT C 0'. On failure (chain returned), calls "
     "sub_115B3 (likely insertDisk-equivalent) and retries."),

    # -- playSound effect table, same shape as OUT.EXE/ULTIMA.EXE. Only
    # one playFX call site exists in this executable and it has no
    # literal effectNum visible at the call site, so (unlike OUT.EXE)
    # there isn't enough evidence here to assign semantic names to the
    # 10 handlers -- named by table position only. --

    (0x13F62, "playSound",
     "dispatches through a 10-entry jump table (off_14A96) by "
     "effectNum, matching OUT.EXE/ULTIMA.EXE's playSound exactly."),
    (0x13F7F, "soundEffect0", "PC-speaker tone handler, table index 0."),
    (0x13F97, "soundEffect1", "PC-speaker tone handler, table index 1."),
    (0x13FB5, "soundEffect2", "PC-speaker tone handler, table index 2."),
    (0x13FBC, "soundEffect3", "PC-speaker tone handler, table index 3."),
    (0x13FC3, "soundEffect4", "PC-speaker tone handler, table index 4."),
    (0x13FE3, "soundEffect5", "PC-speaker tone handler, table index 5."),
    (0x13FEA, "soundEffect6", "PC-speaker tone handler, table index 6."),
    (0x14004, "soundEffect7", "PC-speaker tone handler, table index 7."),
    (0x14038, "soundEffect8", "PC-speaker tone handler, table index 8."),
    (0x14051, "soundEffect9", "PC-speaker tone handler, table index 9."),

    # -- graphics primitives, matching OUT.EXE/ULTIMA.EXE by role and
    # (for videoDrawPoint) exact byte size. --

    (0x13D61, "videoDrawPoint",
     "plots a single pixel with CGA/Tandy/EGA-aware bit-packing, "
     "matching ULTIMA.EXE's videoDrawPoint exactly in size (253 "
     "bytes). Called from fillRect and the drawLine-family helpers."),
    (0x13A1D, "drawLineInternal",
     "Bresenham line rasterizer, matching ULTIMA.EXE's "
     "drawLineInternal exactly in size (186 bytes)."),

    # -- second pass: generateCharacter's own point-buy attribute
    # allocation logic -- the actual character-creation game mechanic.
    # See docs/overview.md#character-creation-point-buy-mechanic-decoded. --

    (0x10E32, "decreaseAttribute",
     "Left-arrow handler: decrements the given attribute (min 10) and "
     "refunds a point to _pointsRemaining, redrawing via "
     "updateAttribute. Returns 0/1 for whether the change was applied "
     "(at the floor, does nothing). Confirms the attribute range's "
     "floor is 10, matching writeDefaultAttributes' starting value."),
    (0x10E61, "increaseAttribute",
     "Right-arrow handler: increments the given attribute (max 25, "
     "only if _pointsRemaining > 0) and spends a point, redrawing via "
     "updateAttribute. Mirror of decreaseAttribute; together these "
     "define generateCharacter's point-buy range as 10-25."),
    (0x10DC2, "clearSelectionArrows",
     "blanks the left/right selection-arrow characters at the given "
     "attribute row (columns 0x0B and 0x1E) before "
     "moveSelectedAttrUp/Down redraws them at the new row via "
     "writeSelectionArrows."),

    # -- third pass: finished the CRT cluster (raw _dos_* primitives,
    # the near-heap allocator, and the last findExecutableFile
    # helpers), all confirmed the same way as before -- exact byte-
    # size matches to OUT.EXE plus direct-read confirmation for the
    # ambiguous ones. --

    (0x127D4, "_dos_ioctl_get", "AX=4400h IOCTL GET DEVICE INFORMATION."),
    (0x12C5F, "_dos_ioctl_set", "AX=4401h IOCTL SET DEVICE INFORMATION."),
    (0x12C71, "_dos_creat", "AH=3Ch CREATE A FILE WITH HANDLE (CREAT)."),
    (0x12C82, "_dos_creatnew",
     "AH=5Bh CREATE NEW FILE, confirmed by direct read (shares tail "
     "with _dos_creat)."),
    (0x12C8F, "_dos_open", "AH=3Dh OPEN DISK FILE WITH HANDLE."),
    (0x12CA0, "_dos_close", "AH=3Eh CLOSE A FILE WITH HANDLE."),
    (0x12CAE, "_dos_read", "AH=3Fh READ FROM FILE WITH HANDLE."),
    (0x12CC2, "_dos_lseek", "AH=42h MOVE FILE READ/WRITE POINTER."),
    (0x12CE6, "_dos_creattemp", "AH=5Ah CREATE UNIQUE FILE."),
    (0x12CF7, "_dos_write", "AH=40h WRITE TO FILE WITH HANDLE."),
    (0x12D20, "_exit", "runs the exit hook if set, then INT 21h/4Ch QUIT."),
    (0x1336E, "_dos_getfileattr",
     "AH=43h/AL=00h GET FILE ATTRIBUTES, used as the file-exists check "
     "by findExecutableFile."),
    (0x12971, "_write",
     "text-mode-aware write underneath _flsbuf: seeks to end first "
     "for append-mode streams via _lseek. Confirmed by direct read, "
     "matching OUT.EXE's _write exactly in size (290 bytes)."),
    (0x12EBF, "hasFileExtension",
     "scans a filename backward for '.', matching OUT.EXE's "
     "hasFileExtension exactly in size (129 bytes)."),
    (0x12F40, "ensureFileExtension",
     "appends a default extension if none found, matching OUT.EXE's "
     "ensureFileExtension exactly in size (128 bytes)."),
    (0x13382, "strcpy",
     "plain lodsb/stosb copy-until-null, confirmed by direct read -- "
     "called from findExecutableFile and ensureFileExtension, "
     "matching OUT.EXE's strcpy usage pattern (not strncpy, despite "
     "similar surrounding context)."),
    (0x133AE, "strncpy",
     "bounded copy, confirmed by direct read, called from "
     "hasFileExtension -- matching OUT.EXE's strncpy usage exactly."),

    (0x12D47, "_nmalloc",
     "near-heap allocator: walks the free-list (sentinel word_14930), "
     "splits a block if big enough, else calls _nheapgrow then _nfree "
     "to extend and link in new space. Confirmed by direct read, "
     "called from allocFileBuffer and buildAndChainExecutable -- same "
     "pair of callers as OUT.EXE's _nmalloc."),
    (0x133F9, "_nheapgrow",
     "grows the near heap via SETBLOCK, matching OUT.EXE's "
     "_nheapgrow exactly in size (198 bytes). Only called from "
     "_nmalloc."),
    (0x11E2C, "_nfree",
     "near-heap free: coalesces into the free-list. Called from "
     "_nmalloc (to link newly-grown space), _openfile-family code, "
     "and _fclose-equivalent cleanup -- same role as OUT.EXE's "
     "_nfree."),
    (0x11DA6, "_nheapinit",
     "one-time near-heap setup, matching OUT.EXE's _nheapinit exactly "
     "in call site (CODE XREF: start+28B)."),

    (0x13C8B, "writeNumber",
     "signature matches OUT.EXE's writeNumber exactly at the call "
     "site in updateAttribute: (value, maxDigits=2, paddingChar=' ', "
     "color, isSigned=0). Draws the numeric attribute values and the "
     "points-remaining counter during character creation."),

    # -- fourth pass: remaining CRT/UI helpers plus real game-logic
    # finds (the 4-slot character roster and its full/replace flow). --

    (0x1197D, "_fclose",
     "flushes via _flsbuf, frees the buffer via _nfree, releases the "
     "handle via releaseFileHandle -- confirmed by direct read, "
     "matching OUT.EXE's _fclose exactly. Called from generateCharacter "
     "and readSavegameList directly as well as from _openfile (to "
     "close any previously-open stream on the same FILE*)."),
    (0x11ED5, "releaseFileHandle",
     "looks up and closes the OS handle, clears the slot -- called "
     "only from _fclose, matching OUT.EXE's releaseFileHandle."),
    (0x11F8C, "exit",
     "the real C-standard exit(): flushes/closes every FILE-table "
     "entry (base 0xDB0) then calls _exit. noreturn, matching "
     "ULTIMA.EXE's exit() exactly in shape. Not called from anywhere "
     "visible in this executable (0 static callers) -- present but "
     "apparently unused, like several other CRT siblings found this "
     "session."),

    (0x115B3, "insertDisk",
     "prints 'Insert Ultima I diskette' / 'Hit space to continue...' "
     "and blocks on a space keypress -- called from launchGame when "
     "the exec-chain to OUT.EXE fails, matching the insertDisk retry "
     "pattern from every other chain call found this project."),
    (0x13EED, "clearMessageArea",
     "INT 10h/AH=06h SCROLL PAGE UP to blank a fixed screen region "
     "(rows 0x15-0x18) -- called once from insertDisk to clear space "
     "for its message."),
    (0x13AD7, "drawLoadingFrame",
     "draws a border frame (4 fillRect calls covering the screen "
     "edges) -- called from launchGame right before printing 'Please "
     "wait whilst thy game loads'."),

    (0x1413C, "getKeypressRaw",
     "the real single-poll keypress primitive (cursor position via "
     "INT 10h/AH=03h, then INT 21h/AH=06h direct console I/O) -- "
     "matches OUT.EXE's getKeypress body exactly. The pre-existing "
     "name 'getKeypress' in this IDB is actually the polling-loop "
     "wrapper (matching OUT.EXE's getKeypressAndWait), left as-is "
     "since it's already cross-referenced throughout this IDB and "
     "isn't factually wrong (it does return a keypress, just via a "
     "wait loop) -- unlike writeString2_mb's rename last pass, this "
     "one wasn't misleading enough to justify the churn."),
    (0x13E5E, "wait",
     "outer wait wrapper (guards against amount=0, which busy-loops "
     "forever -- a real inert edge case, not something exercised) "
     "around waitTimerTicks."),
    (0x13E77, "waitTimerTicks",
     "the INT 1Ch tick-counting primitive, matching OUT.EXE's "
     "waitTimerTicks exactly (install handler, busy-wait, restore)."),
    (0x141D5, "textCursorAnimate",
     "8x8 bitmap cursor blit with a frame counter, matching "
     "ULTIMA.EXE's drawAnimatedCursor in shape -- named "
     "textCursorAnimate here instead to match OUT.EXE's established "
     "name for this exact role (called from within the raw keypress "
     "primitive to animate the cursor while waiting)."),
    (0x14234, "flushKeyboardBuffer",
     "BIOS ROM ID check + keyboard buffer head/tail reset, matching "
     "ULTIMA.EXE's flushKeyboardBuffer exactly."),

    (0x11B9B, "_fread2",
     "byte-for-byte identical to the already-named _fread at a "
     "different address -- a second copy from a different linked "
     "object file, same duplication pattern as ULTIMA.EXE's "
     "strncpy2/toupper2. Called from readSavegameList."),
    (0x13397, "strlen", "plain repne-scasb length count."),
    (0x1357C, "writeCharacter",
     "BIOS TTY character write (INT 10h/AH=0Eh) -- the low-level "
     "primitive under getName/drawRightArrow/writeLeftArrow's text "
     "output."),
    (0x13D23, "writeNumberChar",
     "BIOS write-char-at-cursor (INT 10h/AH=09h or 0Eh depending on a "
     "flag) using color globals byte_14A64/65 -- writeNumber's "
     "internal digit-output helper."),

    (0x114BB, "showCharacterReplacementMenu",
     "draws ' Character Replacement ' with left/right border arrows "
     "and 'The roster is full.  Type the number of the character to "
     "wish...' -- the flow triggered when starting a new character "
     "and all roster slots are already full. Calls drawCharacterRoster "
     "to list the existing 4 characters to choose a replacement from."),
    (0x11455, "drawCharacterRoster",
     "draws up to 4 numbered character names (slots 0-3, STR15-sized "
     "entries from stru_14FC4) -- confirms the savegame roster holds "
     "a maximum of 4 characters. Shared by continuePreviousGame (pick "
     "who to play) and showCharacterReplacementMenu (pick who to "
     "overwrite)."),
]

# (ea, new_name, note)
GLOBAL_RENAMES = [
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

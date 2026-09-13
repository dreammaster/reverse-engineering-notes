"""
IDA Pro script: master list of symbol renames (functions + globals) for
BOOTUP.BIN (DOS) -- Ultima III's actual game executable, chained into
from ULTIMA.COM's title screen (see docs/overview.md's "ULTIMA.COM's
real role" section). Companion to apply_renames_ultima.py, same
accumulating-list rationale -- see that file's docstring for the full
convention writeup (naming casing, DRY_RUN policy, scope vs.
apply_structs_bootup.py). Run against ultima_bootup.idb:

    .\\run_ida_script.ps1 -Idb ultima_bootup -ScriptName apply_renames_bootup.py

Before adding a new entry here, check whether the same address in
ultima.idb already has a confirmed name -- BOOTUP.BIN is loaded into
the exact same memory range ULTIMA.COM occupied (both are tiny-model
COM-style images based at paragraph 1000h, offset 100h), and shares its
low-level runtime (drawTileGrid, playSoundEffect, etc. -- see
docs/overview.md). A shared-runtime function at the same address in
both IDBs should get the same name in both, not be re-derived from
scratch.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x14B99, "playSoundEffect", "identical to ultima.idb's playSoundEffect dispatcher."),

    # -- shared low-level runtime, byte-for-byte structurally identical
    # to the functions of the same name in apply_renames_ultima.py
    # (same source, recompiled/relinked into this separate executable at
    # different addresses -- confirmed by reading through this entire
    # cluster and comparing line-by-line against ULTIMA.COM's, not
    # assumed from the names alone). See that file's notes for the full
    # evidence trail per function; only what differs or adds new
    # evidence is repeated here. --

    (0x143EB, "writeString", "identical to ultima.idb's writeString."),
    (0x14401, "writeStringPreserveCx", "identical to ultima.idb's writeStringPreserveCx."),
    (0x14407, "swapCursorPos", "identical to ultima.idb's swapCursorPos (xchg dx,word_11F40, the local _textCursorPos)."),
    (0x1440C, "writeCharacter", "identical to ultima.idb's writeCharacter."),
    (0x14440, "drawCharGlyph", "identical to ultima.idb's drawCharGlyph -- same ds:[charCode*16+33E9h]-relative glyph read (this binary's copy of the offset, matching ultima.idb's 76C9h-relative one structurally)."),
    (0x144B0, "checkDebugModeFlag", "identical stub to ultima.idb's (mov al,0; retn)."),
    (0x14503, "plotPixel2bpp", "identical to ultima.idb's plotPixel2bpp."),
    (0x14528, "drawTileGrid", "identical to ultima.idb's drawTileGrid -- confirmed reused here for both the DEMO.ULT title-screen map (via updateIntroAnimation/drawLogoTileGrid, 6x19 tiles matching DEMO.ULT's 114-byte size exactly) and SHAPES.ULT tile data (byte_11FE9, loaded from SHAPES.ULT in start_0)."),
    (0x145B3, "readLine", "identical to ultima.idb's readLine."),
    (0x14601, "stepTimeSeededPrng", "identical to ultima.idb's stepTimeSeededPrng."),
    (0x1463A, "swapAnimTableRows", "identical to ultima.idb's swapAnimTableRows."),
    (0x14660, "updateLogoAnimationA", "identical to ultima.idb's updateLogoAnimationA."),
    (0x146AF, "updateLogoAnimationB", "identical to ultima.idb's updateLogoAnimationB."),
    (0x14727, "updateLogoAnimationC", "identical to ultima.idb's updateLogoAnimationC."),
    (0x14768, "updateLogoAnimationD", "identical to ultima.idb's updateLogoAnimationD."),
    (0x147C7, "runIdleAnimationTick", "identical to ultima.idb's runIdleAnimationTick."),
    (0x147EA, "pollKeypressAndAnimate", "identical to ultima.idb's pollKeypressAndAnimate."),
    (0x147FD, "getKeypressAndWaitRaw", "identical to ultima.idb's getKeypressAndWaitRaw."),
    (0x14858, "printHexWord",
     "prints AX as 4 hex digits via printHexByte twice (xchg al,ah "
     "bracketing). SAME LOGIC as an anonymous, unnamed code block in "
     "ultima.idb (right before its printHexByte/sub_18B43) that IDA "
     "never recognized as a function there (no confirmed caller found "
     "in ultima.idb) -- here it's a proper named function with real "
     "callers (showCharacterDetails' Hit Points/Experience/Food/Gold "
     "display, showCharacterDetails' Weapon/Armour lookup lines), "
     "resolving that open question: this logic is real, reachable game "
     "code, not dead weight. Worth revisiting ultima.idb with "
     "ida_funcs.add_func() to give its copy the same name -- see "
     "docs/roadmap.md."),
    (0x14863, "printHexByte", "identical to ultima.idb's printHexByte."),
    (0x1487C, "printHexNibble", "identical to ultima.idb's printHexNibble."),
    (0x1488E, "accumulateInputDigit", "identical to ultima.idb's accumulateInputDigit."),
    (0x1498F, "promptForNumberEntry",
     "SAME LOGIC as ultima.idb's orphaned numeric-entry prompt (never "
     "got a proc boundary there, no confirmed caller) -- here it's a "
     "proper function with a real, confirmed caller: getEntryNumber, "
     "used for every 'Entry#' (1-20 roster index) prompt throughout "
     "character/party management. Resolves ultima.idb's open question "
     "about that routine's purpose and reachability -- it's real, "
     "shared game code, just not exercised from ultima.idb's title "
     "screen itself. See docs/roadmap.md."),
    (0x149E0, "saveFile",
     "FCB-based file writer (AH=15h sequential write), mirrors "
     "ultima.idb's loadFile in structure. NEW relative to ultima.idb -- "
     "that title-screen program never writes any file."),
    (0x14A30, "loadFile", "identical to ultima.idb's loadFile."),
    (0x14A83, "openFileWithRetry", "identical to ultima.idb's openFileWithRetry."),
    (0x14ABE, "printStringAt",
     "swapCursorPos to a given dh/dl position, then "
     "writeStringPreserveCx -- structurally identical to ultima.idb's "
     "printErrorPrefix, but used far more broadly here (every single "
     "menu label/value in the entire character-creation and party-"
     "management UI goes through this), so named for its general role "
     "rather than the narrower error-specific name that fit "
     "ultima.idb's one use case."),
    (0x14AC7, "adjustAnimSpeed", "identical to ultima.idb's adjustAnimSpeed."),
    (0x14AEA, "drawLogoTileGrid",
     "identical shape to ultima.idb's drawLogoTileGrid (11x11 grid via "
     "drawTileGrid) but sourced from byte_10109 (176 bytes) instead of "
     "a shuffled animation buffer -- draws the intro-screen decorative "
     "tile image from static data, not the shuffled/animated one."),
    (0x14B05, "computeAnimTableByte", "identical to ultima.idb's computeAnimTableByte."),
    (0x14B20, "WIND_DIRECTION_TABLE",
     "identical mechanism to ultima.idb's WIND_DIRECTION_TABLE -- "
     "verified via ida_bytes.get_word() against all 5 entries, "
     "pointing at aCalmWind/aNorthWind/aEastWind/aSouthWind/aWestWind "
     "below in the same Calm/North/East/South/West selection order."),
    (0x14B2A, "updateWindDisplay", "identical to ultima.idb's updateWindDisplay."),
    (0x14B80, "_soundEnabled", "identical role to ultima.idb's _soundEnabled."),
    (0x14B81, "SOUND_EFFECT_TABLE",
     "identical mechanism to ultima.idb's SOUND_EFFECT_TABLE -- verified "
     "via ida_bytes.get_word() against all 12 entries. Real address is "
     "0x14B81, NOT 0x14BB1 as IDA's own auto-generated array name "
     "('funcs_14BB1') implies -- same lesson as ultima.idb's "
     "funcs_18E91/0x18E61 mismatch, don't trust an auto-name's numeric "
     "suffix as its address."),
    (0x14BB9, "playToneFF", "identical to ultima.idb's playToneFF."),
    (0x14BDB, "playErrorBeep",
     "identical role to ultima.idb's playErrorBeep (effect 0FEh) -- "
     "confirmed call sites: readLine, promptForNumberEntry, "
     "getMenuChoice, openFileWithRetry's 'Wrong Diskette!' prompt."),
    (0x14BFD, "playToneFD", "identical to ultima.idb's playToneFD."),
    (0x14C56, "playToneFC", "identical to ultima.idb's playToneFC."),
    (0x14C7F, "playToneFB", "identical to ultima.idb's playToneFB."),
    (0x14C9A, "playToneFA", "identical to ultima.idb's playToneFA."),
    (0x14CB8, "playToneF9", "identical to ultima.idb's playToneF9."),
    (0x14CC2, "playToneF8", "identical to ultima.idb's playToneF8."),
    (0x14CE8, "playToneF7", "identical to ultima.idb's playToneF7."),
    (0x14CF2, "playToneF6", "identical to ultima.idb's playToneF6."),
    (0x14CFC, "playToneHelper",
     "shared tone-sweep primitive, same role as ultima.idb's "
     "playToneHelper -- NOT itself in SOUND_EFFECT_TABLE, called "
     "directly by playToneF7/playToneF6 with fixed parameters."),
    (0x14D1E, "playToneF5", "identical to ultima.idb's playToneF5."),
    (0x14D4E, "playToneF4", "identical to ultima.idb's playToneF4."),

    (0x11F5D, "aCalmWind", "'\\x10Calm  Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x11F6A, "aNorthWind", "'\\x10North Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x11F77, "aSouthWind", "'\\x10South Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x11F84, "aEastWind", "'\\x10East  Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x11F91, "aWestWind", "'\\x10West  Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),

    # -- BOOTUP.BIN-specific game logic: character creation, roster, and
    # party management. Confirms the ROSTER.ULT/PARTY.ULT byte layouts
    # documented in docs/file-formats.md field-for-field (every offset
    # below was read directly off a real struct-access instruction, not
    # inferred) -- see docs/overview.md's findings log for the full
    # cross-reference writeup. --

    (0x1120A, "titleScreenAndMainMenuLoop",
     "was start_0. Loads CHARSET.ULT/SHAPES.ULT/DEMO.ULT/MOVES.ULT/"
     "ROSTER.ULT, draws the intro story screen ('(C)-1983 By James R. "
     "Van Artsdalen and Lord British', 'From the depths of hell... "
     "...he comes for VENGEANCE!'), runs updateIntroAnimation 6x per "
     "poll while waiting for a keypress, then loads PARTY.ULT and "
     "enters showMainMenu. The '(C)-1983' credit confirms this "
     "codebase's origin predates the 1991 file dates on disk -- see "
     "docs/roadmap.md's open question about DOS-port vs. original-"
     "release dating."),

    (0x1132B, "drawWindowBorder",
     "draws a decorative box-frame pattern at a given screen position "
     "(dx), reused for every menu screen in this program (title intro, "
     "main menu, party organization, etc.) via a fixed XOR-pattern "
     "sequence against drawTileGrid's/plotPixel2bpp's addressing."),

    (0x11385, "clearMenuWindow",
     "INT 10h AH=6 (scroll page = clear), fixed region (row 11-22, "
     "col 1-38) -- the standard 'clear the menu content area, leave "
     "borders' used by every screen in this program before drawing new "
     "content."),

    (0x113A0, "showMainMenu",
     "the top-level menu after the title screen: 'Return to the View' "
     "(R)/'Organize a Party' (O)/'Journey Onward' (J) -- dispatches to "
     "handleJourneyOnward or showPartyOrganizationMenu via "
     "getMenuChoice."),

    (0x11437, "handleJourneyOnward",
     "'Journey Onward' -- gates on byte_101C3 (party size) and "
     "hasActiveParty (isCharacterAlive-checked party), then loads and "
     "chains into EXODUS.BIN via a self-modifying stub written to "
     "ds:0FAh-0FFh: 'CD 21' (INT 21h, AH=14h FCB sequential read of "
     "the whole file, same DTA=100h-over-own-code trick as ULTIMA.COM "
     "-> BOOTUP.BIN) immediately followed by 'FF 26 28 13' (JMP WORD "
     "PTR [1328h] -- an INDIRECT jump through a 2-byte vector stored "
     "at a FIXED offset within EXODUS.BIN's own freshly-loaded bytes, "
     "not a direct fall-through like the BOOTUP.BIN chain-load). This "
     "means EXODUS.BIN self-describes its own entry point via a "
     "pointer at file offset 0x1228 (0x1328 - 0x100 DTA base), unlike "
     "BOOTUP.BIN which just starts executing at offset 0. Major "
     "finding for docs/overview.md -- EXODUS.BIN needs its own IDB, "
     "see docs/roadmap.md."),

    (0x114C9, "showPartyOrganizationMenu",
     "'Party Organization' -- Examine the Register (E) / Create a "
     "Character (C) / Form the Party (F) / Disperse the Party (D) / "
     "Terminate a Character (T) / Look at a Character (L) / Main Menu, "
     "dispatches via getMenuChoice."),

    (0x1158F, "showCharacterDetails",
     "'Look at a Character' -- prompts for a 1-20 Entry# via "
     "getEntryNumber, then reads and displays the full ROSTER.ULT "
     "character record: Name (+0x00), Sex (+0x18), Race (+0x16), Type/"
     "Class (+0x17), Status (+0x11), Weapon (+0x30), Armour (+0x28), "
     "Strength/Dexterity/Intelligence/Wisdom (+0x12/13/14/15), Hit "
     "Points (+0x1A, printHexWord as BCD), Experience (+0x1E), Food "
     "(+0x21), Gold (+0x23) -- every offset matches "
     "docs/file-formats.md's externally-sourced ROSTER.ULT layout "
     "exactly, confirming that documentation field-for-field against "
     "real code for the first time."),

    (0x11792, "handleFormParty",
     "'Form the Party' -- prompts up to 4 Entry# selections (rejecting "
     "already-selected/empty/dead entries), building the party in "
     "byte_101C3 (count, global _partySize) and byte_101C6 (a 4-byte "
     "array of chosen roster indices), then on completion calls "
     "syncRosterToPartyScratch and writes PARTY.ULT + ROSTER.ULT via "
     "saveFile."),

    (0x118AF, "clearPartySelection",
     "resets all currently-selected party slots' ROSTER.ULT membership "
     "flag (+0x10) back to 0 and zeroes _partySize -- the 'undo "
     "everything selected so far' helper shared by handleFormParty's "
     "'No Active Players' bail-out and handleDisperseParty."),

    (0x118DF, "handleDisperseParty",
     "'Disperse the Party' -- syncPartyScratchToRoster (write back any "
     "in-the-field changes), clearPartySelection, zeroes the "
     "byte_101CE party-record scratch area, rewrites PARTY.ULT/"
     "ROSTER.ULT via saveFile."),

    (0x1194B, "showRegister",
     "'Examine the Register' -- lists all 20 ROSTER.ULT entries "
     "(active-in-party marked '-', currently-selected-for-forming "
     "marked '*'), showing Sex/Race/Type/Status plus the name, paged "
     "in groups that scroll the display."),

    (0x119DB, "handleTerminateCharacter",
     "'Terminate a Character' -- prompts an Entry#, refuses if the "
     "slot is empty or currently in a party ('(With a party)'), "
     "otherwise zeroes the 64-byte record and rewrites ROSTER.ULT."),

    (0x11A6D, "handleCreateCharacter",
     "'Create a Character' -- prompts an Entry#, refuses if not empty "
     "('(Not Empty!)'), otherwise zeroes the record, runs "
     "drawCharacterCreationForm + gatherCharacterCreationInput, "
     "confirms (Y/N), and on Y sets starting values: Status='G' "
     "(Good), party membership=0, HP/Max HP/Food/Gold all "
     "150 (0x150, BCD) via printHexWord's byte layout, Weapon=1 "
     "(Mace), Armour=1 (Cloth), then writes ROSTER.ULT."),

    (0x11B47, "drawCharacterCreationForm",
     "draws the fixed labels for character creation: Entry#, "
     "'Points:50' (the 50-point attribute pool), Name/Sex/Race/Type, "
     "Strength/Dexterity/Intelligence/Wisdom (dotted-line labels), "
     "O.K.?."),

    (0x11BC8, "gatherCharacterCreationInput",
     "collects Name (via readLine, 9 chars), Sex/Race/Type (via "
     "getMenuChoice against fixed option tables), then interactively "
     "allocates the 50-point attribute pool across Strength/Dexterity/"
     "Intelligence/Wisdom via getEntryNumber with live remaining-points "
     "feedback (printRemainingPoints) and per-attribute min/max BCD "
     "range checks (Str >=5, Dex >=5 with >=0x10 remaining, Int >=5 "
     "with points remaining and non-negative, Wis requires enough "
     "pool left) -- classic Ultima character-creation stat allocation, "
     "now with confirmed exact rules for Ultima III specifically."),

    (0x11CD4, "printRemainingPoints",
     "redraws the attribute-pool remaining-points display (cl) at a "
     "fixed position during gatherCharacterCreationInput's allocation "
     "loop."),

    (0x11CE7, "waitForSpaceKey",
     "prints the '<Press Space>'-style prompt (byte_10CB6) and blocks "
     "until Space is pressed -- the universal 'press a key to "
     "continue' used at the end of every menu action in this program."),

    (0x11D07, "syncPartyScratchToRoster",
     "for each active party slot (0.._partySize), copies the "
     "byte_101CE embedded party-record scratch data (PARTY.ULT's "
     "offset 0x12, matching docs/file-formats.md's 4 embedded "
     "character records) back into the real ROSTER.ULT-backed record "
     "via getRosterEntryPointer -- writes back any changes made while "
     "the party was active (HP/gold/XP from actual play) before "
     "dispersing."),

    (0x11D34, "syncRosterToPartyScratch",
     "the inverse of syncPartyScratchToRoster: copies each selected "
     "roster entry's current data into the byte_101CE embedded party-"
     "record scratch area, called when finalizing 'Form the Party' so "
     "PARTY.ULT's embedded records start in sync with ROSTER.ULT."),

    (0x11D61, "getRosterEntryPointer",
     "bl = 1-based roster index (1-20) -> bx = pointer into the "
     "byte_102CE roster array ((index-1)*0x40 + 0x2CE) -- the core "
     "roster-record-lookup helper used throughout this program."),

    (0x11D71, "updateIntroAnimation",
     "per-poll animation step for the title/intro screen specifically: "
     "updateLogoAnimationA/B/C (no D, no wind, no sound -- unlike the "
     "shared runtime's runIdleAnimationTick) then a direct drawTileGrid "
     "call rendering DEMO.ULT (byte_107D2, 114 bytes = exactly 6x19 "
     "tiles, ah=6/al=13h) over the SHAPES.ULT tile data -- confirms "
     "DEMO.ULT's external-doc-sourced '19x6 tiles' size exactly "
     "(docs/file-formats.md)."),

    (0x11D95, "isCharacterAlive",
     "checks a roster/party record's Status byte (+0x11) is 'G' "
     "(Good) or 'P' (Poisoned) -- both playable; anything else (Dead/"
     "Ashes) returns false. Used by hasActiveParty."),

    (0x11DA8, "hasActiveParty",
     "scans up to 4 party slots (byte_101CE) via isCharacterAlive, "
     "returns ah=0xFF if none are alive, else a nonzero count-ish "
     "value -- the gate for handleJourneyOnward's 'no active players' "
     "check and handleFormParty's completion check."),

    (0x11E0C, "getEntryNumber",
     "wraps promptForNumberEntry with a fixed starting cursor "
     "(word_11F40's saved value) -- the 'Entry#' numeric prompt used "
     "throughout character/roster/party selection, returns the "
     "validated 0-99 decimal value in ah (per promptForNumberEntry's "
     "dx accumulator, truncated)."),

    (0x14918, "getMenuChoice",
     "generic single-key menu selector: waits for a keypress via "
     "getKeypressAndWaitRaw, uppercases it, checks membership in a "
     "caller-supplied valid-key list (di, cx entries), beeps "
     "(playErrorBeep) and retries on an invalid key, then prints the "
     "matching label from a parallel string-pointer table (si) before "
     "returning the chosen key in al. The shared driver behind every "
     "menu in this program (showMainMenu, showPartyOrganizationMenu, "
     "gatherCharacterCreationInput's Sex/Race/Type pickers, the Y/N "
     "confirm in handleCreateCharacter)."),
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
    if not RENAMES:
        print("[no-op] RENAMES is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()

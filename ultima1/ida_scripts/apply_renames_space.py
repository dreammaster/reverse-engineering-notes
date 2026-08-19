"""
IDA Pro script: master list of symbol renames (functions + globals) for
SPACE.EXE (ultima1_space.idb).

Single accumulating script per the convention established for the
other three executables. Whenever a finding is confirmed, add an
entry to RENAMES below and re-run.

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_space -ScriptName apply_renames_space.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- first pass: SPACE.EXE links the same CRT as the other three
    # executables. Unlike those, prior work (before this session) had
    # already named much of it with a different style (dosOpenOrCreate,
    # fclose, fillReadBuffer, etc.) -- this pass only fills the actual
    # gaps, confirmed via the same INT 21h subfunction mapping used
    # throughout this project, not by guessing. Two placeholder names
    # from prior work ("exec?", "f") are also cleaned up here since
    # they were clearly tentative, not final. See
    # docs/overview.md#spaceexe-crt-gaps-filled. --

    (0x14259, "_dos_ioctl_get", "AX=4400h IOCTL GET DEVICE INFORMATION."),
    (0x14756, "_dos_open", "AH=3Dh OPEN DISK FILE WITH HANDLE."),
    (0x14767, "_dos_close", "AH=3Eh CLOSE A FILE WITH HANDLE."),
    (0x14775, "_dos_read", "AH=3Fh READ FROM FILE WITH HANDLE."),
    (0x14789, "_dos_lseek", "AH=42h MOVE FILE READ/WRITE POINTER."),
    (0x147AD, "_dos_write", "AH=40h WRITE TO FILE WITH HANDLE."),
    (0x14F4E, "_dos_getfileattr", "AH=43h/AL=00h GET FILE ATTRIBUTES."),
    (0x13F66, "_nheapinit",
     "one-time near-heap setup (SETBLOCK), called from start -- "
     "matches the other three executables' _nheapinit exactly."),
    (0x150F7, "_nheapgrow",
     "grows the near heap via SETBLOCK, confirmed by direct read."),
    (0x14D23, "execProgram",
     "the overlay loader -- 555 bytes, exactly matching the other "
     "three executables' execProgram, and containing the same SETBLOCK "
     "+ PARSE FILENAME + OPEN/READ/CLOSE/LSEEK/CLOSE/SETBLOCK shape."),

    (0x1447E, "_flsbuf",
     "flush-buffer-and-write-one-char, matching the other three "
     "executables' _flsbuf exactly in size (728 bytes). Called from "
     "the pre-existing fclose/fillReadBuffer/fwriteRaw."),
    (0x14879, "_nfree",
     "near-heap free, matching _nfree's role and size (141 bytes) "
     "elsewhere in this project."),
    (0x14F8E, "findFileHandleSlot",
     "internal handle-table lookup, matching the established role and "
     "size (73 bytes)."),
    (0x147D6, "_nmalloc",
     "near-heap allocator, matching the established role and size "
     "(151 bytes)."),
    (0x14B45, "allocFileBuffer",
     "lazily allocates a stream's I/O buffer, matching the "
     "established role and size (91 bytes)."),
    (0x14F62, "strcpy",
     "plain unbounded lodsb/stosb copy, confirmed by direct read -- "
     "called from findExecutableFile/ensureFileExtension-equivalents, "
     "matching the strcpy (not strncpy) usage pattern seen in "
     "OUT.EXE/GEN.EXE."),
    (0x14F77, "strlen",
     "plain repne-scasb length count, confirmed by direct read."),
    (0x150AC, "strncpy",
     "bounded copy, confirmed by direct read, called from the "
     "hasFileExtension-equivalent."),
    (0x14FD7, "_lseek",
     "CRT-level seek, matching the established role and size (213 "
     "bytes)."),
    (0x14294, "findExecutableFile",
     "locates the target program file by trying the bare name then "
     "appending default extensions -- called from the pre-existing "
     "'exec?' (see below)."),
    (0x1430D, "buildAndChainExecutable",
     "builds the DOS command-tail buffer then calls execProgram."),
    (0x14922, "hasFileExtension",
     "scans a filename backward for '.', matching the established "
     "role and size (129 bytes)."),
    (0x149A3, "ensureFileExtension",
     "appends a default extension if none found, matching the "
     "established role and size (128 bytes)."),
    (0x14A23, "_write",
     "text-mode-aware write underneath _flsbuf, matching the "
     "established role and size (290 bytes)."),
    (0x151BD, "_filbuf",
     "renamed from the placeholder name 'f' -- refill-buffer-and-"
     "return-one-char, called from the pre-existing "
     "fillReadBuffer/_lseek-equivalent exactly matching _filbuf's "
     "role elsewhere in this project."),
    (0x1401D, "execProgramEntry",
     "renamed from the placeholder name 'exec?' -- findExecutableFile "
     "then buildAndChainExecutable, called from the pre-existing "
     "execWithEnvp (itself matching chainToExecutable's role)."),

    # -- second pass: game-specific finds, confirmed by direct read
    # (some had pre-existing but unpromoted analysis comments). --

    (0x1583A, "drawLine",
     "thin wrapper over the Bresenham core (drawLineInternal) -- name "
     "and description came from a pre-existing but unpromoted analysis "
     "comment: 'Used throughout combat for the laser-bolt effects in "
     "handleFireCommand/alienFiresBack.'"),
    (0x15818, "drawLineTo",
     "draws a line from the cached endpoint of the last drawLine/"
     "drawLineTo call to a new point -- 'line-to' counterpart of "
     "drawLine, per a pre-existing but unpromoted analysis comment."),
    (0x1585B, "drawLineInternal",
     "the actual Bresenham line-drawing algorithm; plots via a "
     "putPixel helper and caches its endpoint so a subsequent "
     "drawLineTo call can continue from where it left off -- per a "
     "pre-existing but unpromoted analysis comment."),

    (0x1142B, "drawSectorMapIcon",
     "given sector coordinates, checks _spaceMap.x.y._enemyCount / "
     "field_6 (docking anchor) / field_2 (hazard/star) in that "
     "priority order and dispatches to the matching marker-drawing "
     "helper, else drawEmptyMarker. The per-cell renderer behind "
     "inform's long-range sector scan/map screen."),
    (0x1127E, "drawEnemyMarker",
     "drawn when _spaceMapCell._enemyCount > 0 for a sector, in "
     "drawSectorMapIcon's dispatch."),
    (0x112F1, "drawStationMarker",
     "drawn when a sector's docking/encounter anchor (field_6, per an "
     "existing inline comment) is set, in drawSectorMapIcon's "
     "dispatch."),
    (0x11349, "drawStarMarker",
     "drawn when a sector's hazard/star position (field_2, per an "
     "existing inline comment) is set, in drawSectorMapIcon's "
     "dispatch."),
    (0x113B3, "drawEmptyMarker",
     "the default case in drawSectorMapIcon's dispatch -- an empty "
     "sector with no enemies, station, or star."),

    (0x103E9, "isqrt",
     "integer square root via repeated subtraction of consecutive odd "
     "numbers (1,3,5,7,... -- the classic sum-of-odds-equals-a-square "
     "identity), counting how many fit. Called from view, likely for "
     "a distance calculation (proximity to a star/hazard or targeting "
     "range)."),

    # -- third pass: the `status` screen's helper cluster, the `view`
    # cockpit's sector-tracking helpers, and the `spaceAce`/`inform`
    # dialog-frame helpers. --

    (0x16530, "setStatusEntryPos",
     "positions the text cursor for status-list entry `index` in a "
     "two-column grid (13 rows per column, x=2 or x=15) via "
     "setTextPos."),
    (0x1656B, "writeStatusValue",
     "writes a 4-digit, '.'-padded number via writeNumber -- the "
     "status screen's numeric stat display (fuel/shield/hits/etc.)."),
    (0x16272, "remapSoundActionIndex",
     "adds a fixed offset (0x5D) to al -- a tiny action-code-to-sound-"
     "effect-table-index remap, called from soundAction."),
    (0x164BD, "writeDotFill",
     "writes N '.' characters via writeCharTty -- a dotted-line/"
     "separator fill helper for the status display."),
    (0x16237, "calcExperienceLevel",
     "_savegame._experience / 1000 + 1 -- the character-level "
     "formula, called once from status."),
    (0x157A9, "strlen2",
     "hand-coded character-count loop, functionally identical to the "
     "CRT strlen but a separate implementation -- used by the status "
     "list drawer (sub_16586), likely for text centering/padding."),
    (0x164E6, "showMorePrompt",
     "shows the ' More ' pagination prompt with arrows, waits for "
     "space via promptPressSpace, then clears the area -- the status "
     "list's page-break prompt when there are more entries than fit "
     "on screen."),
    (0x16586, "drawStatusList",
     "iterates an array of status-log message indices from arg_4 to "
     "arg_6, drawing each nonzero entry via setStatusEntryPos, paging "
     "through showMorePrompt every 25 rows. The status screen's "
     "scrolling message-log renderer."),

    (0x157CC, "getKeypressAndWaitRaw",
     "poll-loop identical in shape to the same-named function in "
     "OUT.EXE: getKeypress + wait(3) if none, wait(1) if got one, "
     "loop until nonzero. Called from promptDiskSwapRetry and "
     "promptPressSpace (both already named)."),

    (0x10FFD, "snapToNearestSector",
     "rounds a coordinate up by 100 if its %100 remainder exceeds 49, "
     "then up by 1000 if its %1000 remainder exceeds 499 -- a grid-"
     "snapping helper shared by the X/Y sector-change trackers below."),
    (0x11031, "updateSectorChangeX",
     "computes the per-frame X sector-boundary-crossing delta from "
     "_cockpitSpeed and the existing sectorChangeX (already named), "
     "snapping via snapToNearestSector and clamping to max 6. Called "
     "from view."),
    (0x11072, "updateSectorChangeY",
     "Y counterpart of updateSectorChangeX, using sectorChangeY."),

    (0x10F3B, "waitForKeypressTimeout",
     "polls getKeypress in a loop up to 0x2710 (10000) iterations, "
     "returning early on any keypress -- a wait-with-timeout, used by "
     "spaceAce's rank-achieved screen."),
    (0x10F62, "drawBorderBox",
     "draws a rectangular border via drawLine + 3x drawLineTo, then "
     "positions the text cursor inside it -- the box-drawing core "
     "behind drawDialogFrame."),
    (0x10FBC, "drawDialogFrame",
     "calls drawBorderBox with the given color -- the dialog-frame "
     "helper shared by spaceAce's rank-achieved screen and inform's "
     "sector-map screen."),

    # -- fourth pass: the last 6 functions, completing SPACE.EXE. --

    (0x1416F, "fputs",
     "writes a string to a stream one char at a time via _flsbuf when "
     "the buffer count runs out (fixed stream value 0x6010) -- called "
     "from invalidShipExit, matching fputs's role elsewhere in this "
     "project."),
    (0x103AE, "parseParamNum",
     "parses a decimal number from a string char-by-char (bx = bx*10 "
     "+ digit, with overflow checks via carry) -- called twice from "
     "start, matching the command-line numeric-argument parser named "
     "the same in ULTIMA.EXE/GEN.EXE (a separate implementation here, "
     "not literally shared code, but the same role)."),
    (0x1405B, "exit2",
     "byte-for-byte identical role to the already-named exit() at "
     "0x10EBD -- flushes/closes every FILE-table entry (base 0x6002) "
     "then presumably calls the lower-level exit primitive, noreturn. "
     "A second duplicate copy from a different linked object file, "
     "same pattern as strncpy2/toupper2/_fread2 seen elsewhere in this "
     "project. Called from main."),
    (0x15EE1, "buildScanlineOffsetTable",
     "precomputes the row-to-framebuffer-offset table, matching the "
     "same-named function's shape in OUT.EXE/ULTIMA.EXE/GEN.EXE "
     "exactly. Called from initVideo."),
    (0x15C82, "scrollTextArea",
     "scrolls/clears a bottom screen region (BIOS INT 10h/AH=06h for "
     "non-CGA, direct CGA memory manipulation otherwise) -- called "
     "from promptDiskSwapRetry and writeString, so likely a general "
     "'scroll if at the bottom of the text area' helper rather than a "
     "single fixed message box."),
    (0x15CF7, "waitVerticalRetrace",
     "polls the CGA status port (0x3DA) for the vertical-sync bit -- "
     "a frame-timing wait, called from cockpitPerFrame (per-frame "
     "update) and hyperjump (likely for a smooth jump animation)."),
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

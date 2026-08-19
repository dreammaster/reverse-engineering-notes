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

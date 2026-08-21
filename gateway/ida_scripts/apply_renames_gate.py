"""
IDA Pro script: master list of symbol renames (functions + globals) for
gate.idb (GATE.EXE / gate_decoded.exe -- title screen and cutscenes).

Single accumulating script per the convention established in the sibling
ultima1 project. Whenever a finding is confirmed, add an entry to RENAMES
below and re-run.

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb gate -ScriptName apply_renames_gate.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- first pass: the color-setting cluster shared between _main's
    # startup screen-clear and the Font_writeChar glyph renderer. See
    # docs/overview.md#gateidb-color-cluster-decoded. --

    (0x29B80, "max_color_index",
     "Set in init_graphics per video mode: 1 (2-color), 0Fh (16-color "
     "EGA), 0FFh (256-color VGA) -- confirmed by direct read of the "
     "3-way mode dispatch. Used everywhere else in this cluster purely "
     "as a clamp ceiling for color values."),
    (0x2F0B0, "current_draw_color",
     "Initialized to max_color_index in init_graphics; the value "
     "clamped-and-stored by setDrawColor (0x1C082). Distinct from "
     "Font_fgColor/Font_bgColor (already named) -- this is the color "
     "used by the non-text box/fill primitives (sub_1C0C4 and its "
     "callees), not glyph rendering."),
    (0x1B300, "Font_setColors",
     "Trivial two-line setter: arg_0 -> Font_fgColor, arg_2 -> "
     "Font_bgColor (both already-named globals). Confirmed by direct "
     "read -- no clamping or other logic."),
    (0x1BEA4, "Font_setColorsClamped",
     "Stores arg_0 raw (word_2F0AE, not renamed yet -- see roadmap.md) "
     "and arg_2 clamped to max_color_index (word_2F0AC, also not "
     "renamed), then calls Font_setColors(arg_0, clamped arg_2). Called "
     "from _main's EXEC-failure path right before the error-message "
     "print (sub_1BED2) -- i.e. this sets the error text's colors."),
    (0x1C082, "setDrawColor",
     "Clamps its one argument to max_color_index, stores the previous "
     "value of current_draw_color in a local (returned in ax), updates "
     "current_draw_color, then calls sub_24F42 (unnamed -- mirrors the "
     "new color into a field of the SCREEN-typed struct instance at "
     "dseg+2CEEh, see roadmap.md). Called directly from _main with 0Fh "
     "(white) right before the box-fill call now understood to be "
     "sub_1C0C4."),

    # -- second pass: the 5 globals _main passes as argv[4]-argv[8] to
    # GATEMAIN.EXE (via _execl -- see docs/overview.md's EXE-chaining
    # section), left unidentified in the first gate.idb session.
    # Resolved from the gatemain.idb side: gatemain_start's own argv
    # parsing (argv[1]->Mouse_enablement, argv[2]->videoMode, both
    # already-named globals there, confirming the positional mapping)
    # showed argv[6] specifically is gatemain's "_streamMode" --
    # confirmed there as Stream_selectHandler's mode-selector argument.
    # Directly corroborated on THIS side too: word_2A25A is set to the
    # literal constant 4 in one code path (line ~22358 of gate.asm),
    # exactly matching Stream_configure's special-cased
    # "cmdline_param6==4" branch on the gatemain.idb side. See
    # docs/overview.md#word_2a256-58-5a-5c-5e-named-to-match-gatemainidbs-argv-parsing. --

    (0x2A25A, "streamMode",
     "argv[6] to GATEMAIN.EXE. Confirmed as gatemain.idb's _streamMode "
     "(the Stream_selectHandler/Stream_configure mode selector, 0/1/2/4) "
     "-- corroborated here by this exact global being set to the "
     "literal value 4 in one code path, matching Stream_configure's "
     "special-cased mode==4 branch traced on the gatemain.idb side."),
    (0x2A256, "gatemainArg4",
     "argv[4] to GATEMAIN.EXE, positionally matching gatemain.idb's "
     "still-unrenamed cmdline_param4 -- named only for the confirmed "
     "cross-IDB position, not a decoded meaning (gatemain.idb's own "
     "session didn't resolve what this one controls either)."),
    (0x2A258, "gatemainArg5",
     "argv[5] to GATEMAIN.EXE, positionally matching gatemain.idb's "
     "still-unrenamed cmdline_param5 -- same caveat as gatemainArg4."),
    (0x2A25C, "gatemainArg7",
     "argv[7] to GATEMAIN.EXE, positionally matching gatemain.idb's "
     "still-unrenamed cmdline_param7 -- same caveat as gatemainArg4. "
     "(Also feeds Stream_configure's mode==4 branch there, alongside "
     "gatemainArg8, as its second/third parameters -- roles not "
     "individually decoded on either side.)"),
    (0x2A25E, "gatemainArg8",
     "argv[8] to GATEMAIN.EXE, positionally matching gatemain.idb's "
     "still-unrenamed cmdline_param8 -- same caveat as gatemainArg4."),
]


def main():
    print(f"DRY_RUN = {DRY_RUN}")
    for ea, new_name, note in RENAMES:
        old_name = idc.get_name(ea)
        if DRY_RUN:
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ({note})")
        else:
            ok = idc.set_name(ea, new_name, idc.SN_NOCHECK)
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ok={ok}")


main()

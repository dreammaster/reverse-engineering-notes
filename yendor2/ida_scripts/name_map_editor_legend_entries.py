"""
Names sub_20523 and sub_20570, a symmetric pair in RunMapEditorScreen
(a pre-existing name from an earlier session) for editing the wall/
floor legend type numbers.

sub_20523 (-> EditWallLegendTypeNumber): prompts (blank field, msg
0x795D) and reads a numeric entry via sub_1D146(cx=4, not traced --
plausibly a 4-digit numeric input field), storing it to word_2E384 on
success, then redraws via sub_2044C + DrawWallTypeLegendRow.

sub_20570 (-> EditFloorLegendTypeNumber): identical shape for the
floor side -- word_2E386, sub_2047B + DrawFloorTypeLegendRow. Falls
through to sub_20523 on one error path (errorCode not 0/2), letting
Tab-like navigation move from the floor field to the wall field.

Run via:
    .\run_ida_script.ps1 name_map_editor_legend_entries.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20523: "EditWallLegendTypeNumber",
    0x20570: "EditFloorLegendTypeNumber",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20523,
    "Reads a numeric wall-type entry (sub_1D146, not traced) into "
    "word_2E384, then redraws via DrawWallTypeLegendRow. Called from "
    "RunMapEditorScreen; falls through from EditFloorLegendTypeNumber "
    "on one error path.",
    False,
)
ida_bytes.set_cmt(
    0x20570,
    "Reads a numeric floor-type entry (sub_1D146, not traced) into "
    "word_2E386, then redraws via DrawFloorTypeLegendRow. Called from "
    "RunMapEditorScreen; falls through to EditWallLegendTypeNumber on "
    "one error path.",
    False,
)

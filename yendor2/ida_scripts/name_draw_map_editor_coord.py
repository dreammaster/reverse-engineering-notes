"""
Names sub_2044C, called from RunMapEditorScreen (the map/legend
editor) at multiple points: draws a zero-padded number (word_2E384)
at a fixed screen position (4,1), white-on-black opaque text, then
skips the first 2 characters of the formatted result before drawing
it (`add bx,2` before writeString) -- plausibly trimming a
fixed-width zero-padded value down to its last 2 significant digits.
Reads as a small coordinate/position readout drawn in the corner of
the editor screen; the exact field word_2E384 represents (row?
column? cursor index?) isn't confirmed.
-> DrawMapEditorCoordinateReadout

Run via:
    .\run_ida_script.ps1 name_draw_map_editor_coord.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2044C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMapEditorCoordinateReadout", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMapEditorCoordinateReadout': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws word_2E384, zero-padded via FormatNumberZeroPadded, at "
    "fixed position (4,1), skipping the first 2 characters of the "
    "formatted result before drawing -- a small coordinate/position "
    "readout in the map editor's corner; the exact meaning of "
    "word_2E384 isn't confirmed. Called from RunMapEditorScreen.",
    False,
)

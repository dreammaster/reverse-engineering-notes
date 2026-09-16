"""
Names sub_2047B, called from RunMapEditorScreen (2 sites) -- draws
the numeric value of word_2E386 (the map editor's currently-selected
floor tile type, per existing comments: set by the 'F' picker handler
and by the floor-type-entry handler, and consumed by
DrawFloorTypeLegendRow) zero-padded via FormatNumberZeroPadded at a
fixed top-of-screen position (0xA4, 1). Sibling readout to the
already-named DrawMapEditorCoordinateReadout. -> DrawMapEditorFloorTypeReadout

Run via:
    .\run_ida_script.ps1 name_draw_floor_type_readout.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2047B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMapEditorFloorTypeReadout", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMapEditorFloorTypeReadout': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws word_2E386 (currently-selected map editor floor tile "
    "type) zero-padded at (0xA4,1) via FormatNumberZeroPadded + "
    "writeString. Sibling of DrawMapEditorCoordinateReadout. Called "
    "from RunMapEditorScreen.",
    False,
)

"""
Names sub_206A0, called from RunMapEditorScreen: redraws the full
visible 40x24 cell grid (the same area FillVisibleAreaWithSelectedTile
floods) -- snaps the current position to a 40x24 grid boundary, then
for every cell calls PersistExploredCell + LoadWorldDatTilePalette +
DrawCellIconPair (all already named).

-> RedrawMapEditorGrid

Run via:
    .\run_ida_script.ps1 name_redraw_map_editor_grid.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x206A0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RedrawMapEditorGrid", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RedrawMapEditorGrid': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Redraws the full visible 40x24 cell grid in the map editor: for "
    "every cell, PersistExploredCell + LoadWorldDatTilePalette + "
    "DrawCellIconPair. Called from RunMapEditorScreen.",
    False,
)

"""
Names sub_20652, called once from RunMapEditorScreen -- paints the
selected tile at the map editor's cursor position, persists it, and
redraws that cell.

Stages the cursor position (word_2E76E/word_2E770) into
word_3293E/word_32940, calls unnamed sub_204F0, then the already-named
PersistExploredCell and LoadWorldDatTilePalette. Writes word_2E496
(the selected tile) into the cell record [si] and commits it via
FileEntry_Write(errorCode=9)+ErrorCheck. Finally aligns the cursor
position down to the nearest 8-pixel grid cell (x/y) and redraws it
via DrawCellIconPair. The single-cell counterpart to
FillVisibleAreaWithSelectedTile's bulk PaintCellAndPersist loop.
-> PaintCursorCellAndPersist

Run via:
    .\run_ida_script.ps1 name_paint_cursor_cell.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x20652
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PaintCursorCellAndPersist", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PaintCursorCellAndPersist': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Paints the selected tile (word_2E496) at the cursor cell "
    "(word_2E76E/word_2E770), persists via FileEntry_Write"
    "(errorCode=9), and redraws it via DrawCellIconPair. Called from "
    "RunMapEditorScreen.",
    False,
)

"""
Names sub_2070C, called once from RunMapEditorScreen -- the overlay/
wall-tile sibling of the just-named PaintCursorCellAndPersist
(byte-for-byte the same structure, different globals): paints the
selected overlay tile at a second cursor position, persists it, and
redraws that cell.

Stages word_2E772/word_2E774 (a second cursor position, distinct from
PaintCursorCellAndPersist's word_2E76E/word_2E770 -- plausibly the
wall/overlay palette cursor vs. the floor palette cursor) into
word_3293E/word_32940, calls sub_204F0, PersistExploredCell,
LoadWorldDatTilePalette, writes word_2E4A2 into the cell record
[si+2] (the overlay field of DrawCellIconPair's "floor+overlay" pair,
as opposed to PaintCursorCellAndPersist's [si] floor field), commits
via FileEntry_Write(errorCode=9)+ErrorCheck, then redraws the
grid-aligned cell via DrawCellIconPair.
-> PaintCursorOverlayCellAndPersist

Run via:
    .\run_ida_script.ps1 name_paint_cursor_overlay_cell.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2070C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PaintCursorOverlayCellAndPersist", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PaintCursorOverlayCellAndPersist': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Overlay/wall-tile sibling of PaintCursorCellAndPersist: paints "
    "word_2E4A2 into the cell record's [si+2] (overlay field) at "
    "cursor word_2E772/word_2E774, persists via "
    "FileEntry_Write(errorCode=9), redraws via DrawCellIconPair. "
    "Called from RunMapEditorScreen.",
    False,
)

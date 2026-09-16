"""
Names sub_2075B, called twice from RunMapEditorScreen -- a debug
overlay gated on word_328C4 bit 0x400 (returns immediately if clear).

When active, scans a 0x28x0x18 (40x24) grid of map-editor tile
positions (x,y derived from word_36CF7/word_36CF9, the editor's
scroll/view origin, divided down to a 40x24-aligned block). For each
cell it calls TryInteractAtPosition(x, y) and picks a single debug
letter based on the result: 'N' if errorCode==4, else 'I' if
word_32DCE bit 0x10 or 0x8 is set, else 'M' if word_328C8 bit 0x80 is
set, else 'C' if [si+2] bit 0x8000 is set (si presumably left
pointing at a record by TryInteractAtPosition) -- and if none of
those match, draws nothing for that cell. Draws the chosen letter via
writeChar at 8px-per-cell spacing. Reads as a map-editor debug
overlay that labels each visible tile with a one-letter code for
what TryInteractAtPosition considers to be there (exact category
meanings for N/I/M/C not independently confirmed).
-> DrawMapEditorInteractionTypeOverlay

Run via:
    .\run_ida_script.ps1 name_map_editor_interaction_overlay.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2075B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMapEditorInteractionTypeOverlay", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMapEditorInteractionTypeOverlay': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Debug overlay gated on word_328C4 bit 0x400. Scans a 40x24 grid "
    "of map-editor tile positions, calling TryInteractAtPosition per "
    "cell and drawing a one-letter code (N/I/M/C, or nothing) based "
    "on errorCode / word_32DCE / word_328C8 / [si+2] flag bits -- "
    "labels what TryInteractAtPosition considers present at each "
    "tile. Called from RunMapEditorScreen.",
    False,
)

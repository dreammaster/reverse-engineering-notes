"""
Names sub_204F0 and its caller sub_20817.

sub_204F0 (called from BrowseWallTilePalette, BrowseFloorTilePalette,
and sub_20817, each passing their own word_3293E/word_32940 shift
amounts): computes, from the party's world position
(word_36CF7/word_36CF9), the 40x24-block-aligned origin of the
current view (align down to the nearest 0x28/0x18 boundary), each
axis further offset by a caller-supplied right-shifted value. Returns
X in ax, Y in bx. Reads as computing which 40x24 map-editor "block"
(page) the cursor currently sits in, at a caller-selectable zoom/
shift level. -> ComputeMapEditorBlockOrigin

sub_20817 (called once from RunMapEditorScreen): clears the status
area, calls ComputeMapEditorBlockOrigin, formats and draws its two
return values as "H<n>" / "V<n>" (block coordinate readouts), waits
for a keypress (WaitForKeypressTickingMusic), then redraws the whole
map editor UI: coordinate readout, floor-type readout, and both
legend rows. -> ShowMapEditorBlockCoordsAndRedraw

Run via:
    .\run_ida_script.ps1 name_map_editor_block_coords_and_redraw.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x204F0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeMapEditorBlockOrigin", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeMapEditorBlockOrigin': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Computes the 40x24-block-aligned origin of the party's current "
    "map-editor view (align word_36CF7/word_36CF9 down to the "
    "nearest 0x28/0x18 boundary, offset by caller-supplied "
    "word_3293E/word_32940 shifted amounts). Returns X in ax, Y in "
    "bx. Called from BrowseWallTilePalette, BrowseFloorTilePalette, "
    "and ShowMapEditorBlockCoordsAndRedraw.",
    False,
)

ea = 0x20817
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowMapEditorBlockCoordsAndRedraw", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowMapEditorBlockCoordsAndRedraw': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Shows 'H<n>'/'V<n>' block-coordinate readouts (via "
    "ComputeMapEditorBlockOrigin), waits for a keypress "
    "(WaitForKeypressTickingMusic), then redraws the full map editor "
    "UI: coordinate readout, floor-type readout, wall/floor legend "
    "rows. Called once from RunMapEditorScreen.",
    False,
)

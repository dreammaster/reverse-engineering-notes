"""
CORRECTION: the previous round named 0x20070 "ShowTileLegend" and
documented it as "a legend/reference screen ... not a level editor",
based on DrawCellIconPair/the two legend strips never writing back to
map data. That was wrong -- checked the highest-ref-count neighboring
functions (via rank_naming_candidates.py) and found the 'A' key inside
this screen (byte_2E400 == 0x41) calls sub_2034B, which loops over a
full 40x24 grid (0x28 x 0x18 -- the whole 320x200 screen at 8x8-pixel
granularity) calling sub_203AC per cell. sub_203AC:
  - calls PersistExploredCell(x, y)
  - calls sub_205C0 (reads a record from WORLD.DAT via FileEntry_Read,
    FileEntry bx=0x9043) to get a pointer si
  - writes the CURRENTLY SELECTED legend icon index (word_2E496) into
    [si]
  - sets errorCode=9 and calls FileEntry_Write -- an actual write back
    to file-backed storage, not just an in-memory redraw
  - redraws that cell via DrawCellIconPair

So 'A' floods the entire visible map area with whatever tile type is
currently selected in the legend strip, and persists it. Other keys in
the same screen fit an editor, not a viewer: '9' (byte_2E400==9) shows
"H"/"V" axis-labeled coordinate readouts (sub_20817, a templated
string with its first char forced to 'H' or 'V' before printing); 'B'
and 'F' (sub_205FB/sub_20626) jump the wall/floor legend strips'
scroll position to a specific palette entry read from WORLD.DAT via
the same sub_205C0/sub_27FE0 path (record index from word_2E776,
"block" identifier from word_31956). This is a per-level tile-palette
browser tied to a fill tool -- a debug/level-editor screen left
reachable from the normal keyboard dispatch, not a documented player
feature.

Renaming the 3 pieces confirmed above (leaving sub_205C0/sub_205FB/
sub_20626/sub_20817/sub_27FE0 -- the WORLD.DAT palette-lookup side --
uninvestigated/unnamed, since their exact record layout isn't nailed
down yet):
  0x20070 ShowTileLegend           -> RunMapEditorScreen
  0x2034B (unnamed)                -> FillVisibleAreaWithSelectedTile
  0x203AC (unnamed)                -> PaintCellAndPersist

Run via:
    .\run_ida_script.ps1 fix_tile_legend_is_editor.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20070: "RunMapEditorScreen",
    0x2034B: "FillVisibleAreaWithSelectedTile",
    0x203AC: "PaintCellAndPersist",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20070,
    "CORRECTED from 'ShowTileLegend' (was wrongly documented as a "
    "read-only legend screen). Reached from a normal keyboard command "
    "slot in start's main dispatch. Draws two scrollable 17-icon legend "
    "strips (wall table 0xE551, floor table 0xE175) and a live preview "
    "of the current cell. Its 'A' key (byte_2E400==0x41) calls "
    "FillVisibleAreaWithSelectedTile, which floods the entire visible "
    "40x24 cell area with the selected legend icon and writes it back "
    "via FileEntry_Write -- this IS a map-editing tool (a debug/level-"
    "editor screen left reachable in the shipped binary), not a passive "
    "legend. 'B'/'F' browse a per-level tile palette loaded from "
    "WORLD.DAT (sub_205C0/sub_27FE0, not yet fully traced).",
    False,
)
ida_bytes.set_cmt(
    0x2034B,
    "'A' key handler in RunMapEditorScreen: loops over the full 40x24 "
    "visible cell grid (320x200 screen at 8x8-pixel granularity), "
    "calling PaintCellAndPersist for every cell -- floods the whole "
    "visible map area with the currently-selected legend tile type.",
    False,
)
ida_bytes.set_cmt(
    0x203AC,
    "Per-cell paint: PersistExploredCell(x,y), looks up a WORLD.DAT-"
    "backed record via sub_205C0, writes the current legend selection "
    "(word_2E496) into it, saves via FileEntry_Write (errorCode=9), "
    "then redraws the cell (DrawCellIconPair). Called per-cell by "
    "FillVisibleAreaWithSelectedTile.",
    False,
)

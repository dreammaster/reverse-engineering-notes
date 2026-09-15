"""
Traces the sub_20070 cluster, reached from a normal main-loop keyboard
command slot (`start`+~0x9D1, same dispatch style as every other
single-key command). Confirmed pieces:

- sub_20888: given es:bx pointing at a map cell (same 8-byte record
  GetMapCellPtr addresses), draws that cell's floor icon
  (g_pictureDir entry from table 0xE551 field +0xA, indexed by
  es:[bx]) then, if es:[bx+2] is nonzero, an overlay/wall icon
  (g_pictureDir entry from table 0xE175 field +8, indexed by
  es:[bx+2]) on top with transparency on. This is the exact same
  floor+overlay composite BuildMinimapTileData/DrawMinimap use per
  cell, just drawn at full (unscaled) size at the current x/y instead
  of into the 8x8 minimap grid. -> DrawCellIconPair

- sub_20406: draws 0x11 (17) consecutive icons from table 0xE551
  (field +0xA) as a horizontal strip at y=0, x starting at 0x18,
  starting from index word_2E384 (so it's a scrollable/offset window,
  not necessarily the table's first 17 entries). -> DrawWallTypeLegendRow

- sub_204AA: mirrors sub_20406 for table 0xE175 (field +8), strip at
  y=0, x starting at 0xB8, starting from index word_2E386.
  -> DrawFloorTypeLegendRow

- sub_20C7C: fuzzy/paired equality -- ax==bx, or ax==bx after nudging
  ax to its even/odd pair partner (ax+1 if ax even, ax-1 if ax odd).
  Lets a caller treat two adjacent table indices (e.g. a base type and
  its variant) as an equivalent match. -> IsPairedValueMatch

sub_20070 itself: sets up video state, draws two label strings
(word_2E384/word_2E386 as string-table ids, via sub_2044C/sub_2047B),
draws both legend strips (sub_20406/sub_204AA), converts a stored
screen position (word_36CF7/word_36CF9) into a map cell via
GetMapCellPtr, and draws that cell's icon pair (sub_20888) as a live
preview -- then runs its own PollKeyboardInput loop (ESC exits via a
full redraw + BuildMinimapTileData/DrawMinimap/DrawMouseCursor,
matching the normal post-command refresh pattern; other error codes
branch elsewhere, not traced). No write-back to the map data was
found anywhere in this cluster (sub_20888 and the legend strips only
ever read table/map data) -- this reads as a reference/legend screen
for decoding the automap's tile icons, not a level editor, though the
exact manual name/key for it isn't confirmed. -> ShowTileLegend
(moderate confidence on the overall role; high confidence on the
individual pieces above)

The sub_20C8E/sub_20CEC/sub_20D2F/sub_20E12/sub_29FF6 sibling cluster
(called from the same sub_20C1E master-redraw dispatch, and reusing
these same two tables) draws two small "current cell class" preview
boxes plus scans candidate lists to highlight the matching legend
icon -- structure is understood (see docs/overview.md) but the exact
per-field semantics (word_328E6.. word_328F2, the 0xE551 table's +0/+2
fields) are NOT confirmed enough to name; left alone this round.

Run via:
    .\run_ida_script.ps1 name_tile_legend.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20888: "DrawCellIconPair",
    0x20406: "DrawWallTypeLegendRow",
    0x204AA: "DrawFloorTypeLegendRow",
    0x20C7C: "IsPairedValueMatch",
    0x20070: "ShowTileLegend",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20888,
    "Draws one map cell's icon pair: floor (g_pictureDir via table "
    "0xE551 field +0xA, indexed by es:[bx]) then, if es:[bx+2] != 0, "
    "an overlay/wall icon (g_pictureDir via table 0xE175 field +8, "
    "indexed by es:[bx+2]) drawn transparently on top. Same composite "
    "BuildMinimapTileData/DrawMinimap use per cell, but full-size.",
    False,
)
ida_bytes.set_cmt(
    0x20406,
    "Draws a scrollable 17-icon horizontal strip from table 0xE551 "
    "(field +0xA), starting at index word_2E384, at y=0 x=0x18+.",
    False,
)
ida_bytes.set_cmt(
    0x204AA,
    "Draws a scrollable 17-icon horizontal strip from table 0xE175 "
    "(field +8), starting at index word_2E386, at y=0 x=0xB8+.",
    False,
)
ida_bytes.set_cmt(
    0x20C7C,
    "Fuzzy/paired equality: returns ax==bx, or (ax's even/odd pair "
    "partner)==bx -- i.e. ax+1==bx if ax is even, ax-1==bx if ax is "
    "odd. Lets a caller treat two adjacent table indices as a match.",
    False,
)
ida_bytes.set_cmt(
    0x20070,
    "Interactive legend/reference screen, reached as a normal main-loop "
    "keyboard command (not confirmed which manual key). Draws two "
    "scrollable 17-icon legend strips (wall table 0xE551, floor table "
    "0xE175) plus a live preview of the current cell's icon pair "
    "(DrawCellIconPair via GetMapCellPtr on word_36CF7/word_36CF9). "
    "Own PollKeyboardInput loop; ESC exits via the normal full-redraw "
    "path. No writes back to map data found -- reads as a legend/key "
    "screen for the automap symbols, not an editor. Moderate confidence "
    "on the overall role.",
    False,
)

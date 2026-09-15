"""
Names sub_205FB and sub_20626, RunMapEditorScreen's 'B'/'F' palette-
browsing key handlers (mentioned in file-formats.md's existing
map-editor writeup): pick a wall/floor type from the per-level tile
palette (LoadWorldDatTilePalette) at the clicked position
(word_2E776/word_31956 -> sub_204F0, an index computation, not
traced), storing the result into the same fields
EditWallLegendTypeNumber/EditFloorLegendTypeNumber write, then
redrawing the corresponding legend row.

sub_205FB (-> BrowseWallTilePalette): wall side, [si] -> word_2E384/
word_2E496, DrawWallTypeLegendRow.

sub_20626 (-> BrowseFloorTilePalette): floor side, [si+2] ->
word_2E386/word_2E4A2, DrawFloorTypeLegendRow.

Run via:
    .\run_ida_script.ps1 name_browse_tile_palette.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x205FB: "BrowseWallTilePalette",
    0x20626: "BrowseFloorTilePalette",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x205FB,
    "'B' key handler: picks a wall type from the tile palette at the "
    "clicked position (LoadWorldDatTilePalette), stores it to "
    "word_2E384/word_2E496 (same fields EditWallLegendTypeNumber "
    "writes), redraws via DrawWallTypeLegendRow. Called from "
    "RunMapEditorScreen.",
    False,
)
ida_bytes.set_cmt(
    0x20626,
    "'F' key handler, floor counterpart to BrowseWallTilePalette: "
    "picks a floor type from the palette, stores to word_2E386/"
    "word_2E4A2, redraws via DrawFloorTypeLegendRow. Called from "
    "RunMapEditorScreen.",
    False,
)

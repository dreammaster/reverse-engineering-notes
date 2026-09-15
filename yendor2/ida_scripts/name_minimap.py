"""
Names the dungeon minimap rendering pair, found by following the
sub_209D2/sub_20C1E/sub_21612/sub_21588 sequence that recurs after
every state-changing action in `start`'s main loop:

- sub_21612: gathers a 7x9 grid of tile render data centered 3 rows up
  / 4 cols left of the player (`word_36CF9-3`, `word_36CF7-4`) into a
  local buffer (si=0xD06, 4 bytes/cell = 2 words). For each map cell
  (GetMapCellPtr-style addressing), if its explored flag ([+6] bit
  0x8000) is set, looks up two picture ids from its [+0]/[+2] fields
  via two lookup tables (12 bytes/entry @ 0xE551, 10 bytes/entry @
  0xE175); unexplored cells get a fixed blank-tile default (0x13, 0).
  -> BuildMinimapTileData
- sub_21588: draws that 7x9 grid (reading the SAME si=0xD06 buffer) as
  8x8-pixel `DrawPicture` calls at (0xF0 + 8*col, 8 + 8*row) -- a base
  tile ([si], _font_bgTransparent=3) plus an optional overlay ([si+2],
  _font_bgTransparent=2, skipped if 0). This is a small on-screen
  minimap widget (top-left-ish fixed position), not a full-screen
  first-person view -- the game's dungeon "rendering" is this
  tile-grid minimap. -> DrawMinimap

Run via:
    .\run_ida_script.ps1 name_minimap.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x21612: "BuildMinimapTileData",
    0x21588: "DrawMinimap",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x21612,
    "Gathers a 7x9 grid of tile render data (2 picture ids per cell) "
    "centered on the player into a local buffer (0xD06), from "
    "GetMapCellPtr-style map cells: explored cells look up their "
    "picture ids via two tables ([+0] -> 0xE551, [+2] -> 0xE175); "
    "unexplored cells get a fixed blank default. Feeds DrawMinimap.",
    False,
)
ida_bytes.set_cmt(
    0x21588,
    "Draws the 7x9 minimap grid BuildMinimapTileData just built (same "
    "0xD06 buffer): base tile + optional overlay per cell, 8x8 pixels "
    "each, at a fixed on-screen position. The dungeon view is this "
    "small tile-grid minimap widget, not a full-screen first-person "
    "render.",
    False,
)

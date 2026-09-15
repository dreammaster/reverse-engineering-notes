"""
Names a coherent cluster discovered by following the movement-position
update in sub_112AE (called directly from `start`) into sub_21D30 --
the dungeon map's fog-of-war/exploration-reveal system:

- sub_16F64: computes a map cell's linear address --
  `bx = (y - word_2E564)*0x270 + (x - word_2E55C)*8`, es=word_2E562
  (the map data segment). Confirms an 8-byte-per-cell grid, row stride
  0x270 = 78 cells wide (0x270/8). -> GetMapCellPtr
- sub_21DBB: looks up a cell via GetMapCellPtr; if its "explored" flag
  (bit 0x8000 at +6) is already set, does nothing; otherwise sets it
  and calls sub_21CC2 (the actual reveal/render action for a
  newly-discovered cell, not yet traced) -- classic
  visit-once-then-skip fog-of-war marking. -> MarkCellExplored
- sub_21D8D: calls MarkCellExplored for (x-1,y), (x+1,y), (x,y) at the
  current word_36CF9 -- scans 3 cells varying word_36CF7 (x).
  -> ScanAdjacentCellsAlongX
- sub_21DA4: same shape varying word_36CF9 (y).
  -> ScanAdjacentCellsAlongY
- sub_21D30: the direction-aware entry point (word_36CF5, the same
  facing-direction flags ProbeFacingTile/TryInteractAtPosition use):
  reveals the cells to both sides of the player's facing direction by
  temporarily shifting position and calling the X/Y scanners. Called
  right after the player's position updates. -> RevealCellsAroundPlayer

Matches the manual's automap feature ("M uses the party map") --
cells become known as you walk near them.

Run via:
    .\run_ida_script.ps1 name_map_reveal.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x16F64: "GetMapCellPtr",
    0x21DBB: "MarkCellExplored",
    0x21D8D: "ScanAdjacentCellsAlongX",
    0x21DA4: "ScanAdjacentCellsAlongY",
    0x21D30: "RevealCellsAroundPlayer",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x16F64,
    "Map cell linear address: (y-word_2E564)*0x270 + (x-word_2E55C)*8, "
    "es=word_2E562 (map data segment). 8 bytes/cell, row stride 0x270 "
    "= 78 cells wide.",
    False,
)
ida_bytes.set_cmt(
    0x21DBB,
    "If the cell's explored flag (bit 0x8000 at +6) isn't set, sets it "
    "and calls sub_21CC2 (reveal/render action, not traced) -- "
    "fog-of-war visit-once marking.",
    False,
)
ida_bytes.set_cmt(
    0x21D30,
    "Reveals the map cells to both sides of the player's facing "
    "direction (word_36CF5) around the current position -- called "
    "right after the player's position updates. The automap's "
    "'cells become known as you walk near them' mechanic.",
    False,
)

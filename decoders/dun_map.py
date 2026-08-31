#!/usr/bin/env python3
"""Decode `DUNM1/2/3.BSV` (dungeon tile maps) and `MUSDATA.BSV` (the
museum's exhibit maps) -- both 16x16 tile levels read by the same
first-person view renderer (`bmDUNG` == `bmMUSDUNG`).

    python decoders/dun_map.py [DUNM1|MUSDATA] [C:\\games\\lota]

`MUSDATA.BSV` holds 3 museum floor maps (16x16) at `0x000`, then -- from
`0x800` on -- a near-copy of `DUNDATA.BSV`'s dungeon-view tile/graphic
data (so the museum halls render exactly like dungeon corridors). Tile
`0xE0`-`0xEF` on a museum map = one of the 16 exhibit / display-case
portals `enterExhibit` routes on.

Each file is a Microsoft BASIC `BSAVE` image (`[FD][seg][off][len]`,
7-byte header; here seg 0x2C07, off 0x0F3C).  The payload is exactly
**2048 bytes = 8 dungeon levels x 16 x 16 tiles, 1 byte per tile**,
row-major.  `DUN.EXE`'s `loadDungeonLevel` binds the array at DGROUP
`ds:1E2A` and indexes `base + level*0x100` (256 bytes/level), then
classifies each byte: `>= 0x10` .. wall/normal, `== 0x8F` / `== 0xFF`
solid rock, `< 0x10` a special feature looked up in a table (the
"POISON GAS VENT" / "FLOOR HOLE" / "SLIME SPLOTCH" trap names live in
`DUN.EXE`'s string pool).

Tile bytes seen:
    0x00        open floor / corridor
    0xFF        solid rock (the maze walls)
    0x01..0x0F  special-feature tiles -- doors, up/down stairs, traps,
                treasure, level links.  Exact code->feature mapping
                still TBD (needs `loadDungeonLevel`'s feature table).

`DUNM1/2/3` swap per dungeon group; `DUNDATA.BSV` (loaded contiguously
at 0x2C07:0x173C, right after the map) is the constant part -- see
docs/file-formats.md.
"""
import os
import struct
import sys

LEVELS = 8
W = H = 16
LEVEL_BYTES = W * H

GLYPH = {0x00: "  ", 0xFF: "##"}   # everything else printed as its hex


def read_bsave(path):
    b = open(path, "rb").read()
    if b[0] != 0xFD:
        raise ValueError(f"{path}: not a BSAVE image")
    seg, off, ln = struct.unpack_from("<HHH", b, 1)
    return b[7:7 + ln]


def dump(name, games_dir):
    p = read_bsave(os.path.join(games_dir, name + ".BSV"))
    # MUSDATA.BSV: 3 museum "exhibit" levels (16x16) at the start, then a
    # near-copy of DUNDATA.BSV's dungeon-view graphics (bmMUSDUNG == bmDUNG).
    museum = name.upper().startswith("MUSDATA")
    nlev = 3 if museum else len(p) // LEVEL_BYTES
    print(f"{name}.BSV  payload {len(p)} bytes = {nlev} "
          f"{'exhibit maps' if museum else 'levels'} x {W}x{H}")
    for lvl in range(nlev):
        g = p[lvl * LEVEL_BYTES:(lvl + 1) * LEVEL_BYTES]
        if not any(g):
            continue
        if museum:
            marks = sorted({b for b in g if 0xE0 <= b <= 0xEF})
            tag = ("exhibit markers: "
                   + (", ".join(f"0x{m:02X}" for m in marks) or "(none)"))
        else:
            feats = sorted({b for b in g if 0 < b < 0x10})
            tag = ("feature codes: "
                   + (", ".join(f"0x{f:02X}" for f in feats) or "(none)"))
        print(f"\n--- {'exhibit map' if museum else 'level'} {lvl}   {tag}")
        for r in range(H):
            row = g[r * W:(r + 1) * W]
            print("  " + "".join(GLYPH.get(b, f"{b:02X}") for b in row))


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "DUNM1"
    games = sys.argv[2] if len(sys.argv) > 2 else r"C:\games\lota"
    dump(name, games)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Decode `OUTDATA.BSV`'s terrain-tile records -- the overworld map's
per-terrain-type tile definitions for `OUT.EXE`.

    python decoders/outdata.py [C:\\games\\lota\\OUTDATA.BSV]

`loadOverworldData` `BLOAD`s `OUTM0<n>.BSV` (the map) at `ds:1E2A`
array byte 0 and `OUTDATA.BSV` at array byte `0x2B22` -- one contiguous
array, exactly parallel to the dungeon's `DUNM* + DUNDATA`.

## The terrain records

`OUTDATA` payload `0x000`-`0x3FF` = **256 records x 4 bytes**.  Record
`T` is the four 8x8 **sub-cell indices** for terrain-tile-type `T`, laid
out as a **2x2 grid**:

    byte 0 = top-left      byte 1 = top-right
    byte 2 = bottom-left   byte 3 = bottom-right

Values `0x10`-`0x6D` index the sub-cell bitmap bank carried in
`OUTOBJ.BSV` (loaded into `spriteBank`).  253 of 256 types are defined;
type `0x2C` = `3A 3A 3A 3A` is the plain ground quad, and terrain
transitions come in groups of four (the edge rotations).

## The renderer (parallels the dungeon)

`refreshMapView` walks the 9x9 map window around the player; per tile it
reads `OUTDATA[tile*4 .. +3]` and writes the four sub-cell bytes into a
26-wide tile-index buffer -- TL/TR on one row, BL/BR on the row `0x1A`
below.  `rtm_FE69` then blits that buffer as a `26 x 17` grid of 8x8
cells via **`drawTileRun`** (`rtm_FE2A`), sub-cell bitmaps from the
`OUTOBJ` bank.  `readTileObject` clips partial edge tiles
(`ds:2444h`-`ds:2452h`) with `rtm_FE1B` / `rtm_FE14`.

Note: the `imul ds:2444h, 0x5F` in `resolveMoveTarget` is **not** a
record size -- `0x5F` (95) is the `OUTM` base-terrain grid width, used to
copy a 13x13 map window (`array[0x120 + Y*0x5F + X]`) for move logic.

Open: the exact `OUTOBJ` sub-cell bank base (needs `sub_28156` traced);
the `0x400`-`0xDFF` CGA pixel region's consumer.
"""
import sys

N_TILES = 256
REC = 4


def load(path):
    d = open(path, "rb").read()
    return d[7:] if d[:1] == b"\xFD" else d


def records(p):
    """[(tile, (tl, tr, bl, br)), ...] for the defined terrain types."""
    out = []
    for t in range(N_TILES):
        r = tuple(p[t * REC:t * REC + REC])
        if any(r):
            out.append((t, r))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\OUTDATA.BSV"
    p = load(path)
    recs = records(p)
    print("OUTDATA payload %d B  --  %d of %d terrain-tile records defined\n"
          % (len(p), len(recs), N_TILES))
    print("tile   TL TR / BL BR   (2x2 sub-cell indices)")
    for t, (tl, tr, bl, br) in recs:
        grp = "  <- group of 4" if t % 4 == 0 and t + 3 < N_TILES and all(
            any(p[(t + k) * 4:(t + k) * 4 + 4]) for k in range(4)) else ""
        print("  %02X   %02X %02X / %02X %02X%s" % (t, tl, tr, bl, br, grp))

    # sub-cell usage histogram
    from collections import Counter
    c = Counter(b for _, r in recs for b in r if b)
    print("\nsub-cell indices used: %s"
          % " ".join("%02X(%d)" % (k, v) for k, v in sorted(c.items())))


if __name__ == "__main__":
    main()

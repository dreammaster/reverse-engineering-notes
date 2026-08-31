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

Values `0x01`-`0x91` (+ a `0xDB` "blank" for the unused `0xF0`-`0xFF`
types) index the **sub-cell bitmap bank at OUTDATA payload `0x400`** --
~146 field-interleaved 16-byte 8x8 CGA cells, cell `N` at `0x400 + N*16`
(cell 0 = blank).  253 of 256 types are defined;
type `0x2C` = `3A 3A 3A 3A` is the plain ground quad, and terrain
transitions come in groups of four (the edge rotations).

## The renderer (parallels the dungeon)

`refreshMapView` walks the 9x9 map window around the player; per tile it
reads `OUTDATA[tile*4 .. +3]` and writes the four sub-cell bytes into a
26-wide tile-index buffer -- TL/TR on one row, BL/BR on the row `0x1A`
below.  `rtm_FE69` then blits that buffer as a `26 x 17` grid of 8x8
cells via **`drawTileRun`** (`rtm_FE2A`).  `sub_28156` supplies
`drawTileRun`'s source: `srcSeg` = the `ds:1E2A` array segment (OUTM +
OUTDATA), `srcBase` = array byte `0x2F22` = OUTDATA payload `0x400` (the
override taken when `ds:2082h == 0x23CD`, the overworld tile-buffer
base -- byte `0x479A`).  So the terrain sub-cells live in OUTDATA, not
OUTOBJ; OUTOBJ is only the object / creature `basPutSprite` sprites.
`readTileObject` clips partial edge tiles (`ds:2444h`-`ds:2452h`) with
`rtm_FE1B` / `rtm_FE14`.

Note: the `imul ds:2444h, 0x5F` in `resolveMoveTarget` is **not** a
record size -- `0x5F` (95) is the `OUTM` base-terrain grid width, used to
copy a 13x13 map window (`array[0x120 + Y*0x5F + X]`) for move logic.
"""
import struct
import sys
import os

N_TILES = 256
REC = 4
BANK = 0x400
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


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


def sub_cell(p, n):
    """16-byte bank cell -> 8 rows of 8 palette indices.  Same
    field-interleave as every other 8x8 cell: on-screen row order is
    w0,w4,w1,w5,w2,w6,w3,w7."""
    o = BANK + n * 16
    if o + 16 > len(p):
        return [[0] * 8 for _ in range(8)]
    w = struct.unpack("<8H", p[o:o + 16])
    order = [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]
    return [[(row >> (14 - 2 * x)) & 3 for x in range(8)] for row in order]


def terrain_tile(p, T):
    """The 16x16 image for terrain-tile-type T (2x2 of sub-cells)."""
    r = p[T * REC:T * REC + REC]
    img = [[0] * 16 for _ in range(16)]
    for q, (qy, qx) in enumerate(((0, 0), (0, 1), (1, 0), (1, 1))):
        c = sub_cell(p, r[q])
        for y in range(8):
            for x in range(8):
                img[qy * 8 + y][qx * 8 + x] = c[y][x]
    return img


def write_bmp(path, rows, scale=1):
    h = len(rows)
    ww = len(rows[0]) if h else 0
    W_, H_ = ww * scale, h * scale
    rb = (W_ * 3 + 3) & ~3
    pad = rb - W_ * 3
    size = rb * H_
    with open(path, "wb") as f:
        f.write(b"BM" + struct.pack("<IHHI", 14 + 40 + size, 0, 0, 54))
        f.write(struct.pack("<IiiHHIIiiII", 40, W_, H_, 1, 24, 0, size, 2835, 2835, 0, 0))
        for y in range(H_ - 1, -1, -1):
            srow = rows[y // scale]
            for x in range(W_):
                r, g, b = PAL[srow[x // scale]]
                f.write(bytes((b, g, r)))
            f.write(b"\0" * pad)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\OUTDATA.BSV"
    outdir = sys.argv[2] if len(sys.argv) > 2 else None
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
    print("\nsub-cell indices used (0x400 bank): %s"
          % " ".join("%02X(%d)" % (k, v) for k, v in sorted(c.items())))

    if outdir:
        os.makedirs(outdir, exist_ok=True)
        S, per = 6, 16
        tiles = [t for t, _ in recs]
        cw = 16 * S + 2
        rowsN = (len(tiles) + per - 1) // per
        cv = [[0] * (cw * per) for _ in range(cw * rowsN)]
        for i, T in enumerate(tiles):
            img = terrain_tile(p, T)
            ox, oy = (i % per) * cw, (i // per) * cw
            for y in range(16):
                for x in range(16):
                    v = img[y][x]
                    for dy in range(S):
                        for dx in range(S):
                            cv[oy + y * S + dy][ox + x * S + dx] = v
        out = os.path.join(outdir, "outdata_terrain.bmp")
        write_bmp(out, cv)
        print("wrote", out)


if __name__ == "__main__":
    main()

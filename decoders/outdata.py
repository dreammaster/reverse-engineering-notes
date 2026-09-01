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

Values index the **sub-cell bitmap bank at OUTDATA payload `0x400`**:
**146 cells** (`0x00`-`0x91`), `0x400`-`0xD1F` (`0x920` B), each a
**field-interleaved 16-byte 8x8 CGA cell**, cell `N` at `0x400 + N*16`.
Cell `0x00` = all zero (blank ground = colour 0, black); `0x400 + 146*16`
onward is zero padding to `0x1400`.  The terrain records reference every
cell `0x01`-`0x91`, plus `0x00` and `0xDB` (which lands in the zero pad
= blank) as the filler for the undefined `0xF0`-`0xFF` tile types.
253 of 256 tile types are defined; type `0x2C` = `3A 3A 3A 3A` is the
plain-ground quad (cell `0x3A` = the grass dither), and terrain
transitions come in **groups of four** = the 4 edge rotations
(coastline, forest edge, mountain edge, snow/desert edge).  The `main()`
render (pass an outdir) writes `outdata_subcells.png` (the 146-cell
bank) and `outdata_terrain.bmp` (the 253 assembled 16x16 tiles).

## The renderer (parallels the dungeon)

`refreshMapView` walks the 9x9 map window around the player; per tile it
reads `OUTDATA[tile*4 .. +3]` and writes the four sub-cell bytes into a
26-wide tile-index buffer -- TL/TR on one row, BL/BR on the row `0x1A`
below.  `rtm_FE69` then blits that buffer as a `26 x 17` grid of 8x8
cells via **`drawTileRun`** (`rtm_FE2A`).  `sub_28156` supplies
`drawTileRun`'s source: `srcSeg` = the `ds:1E2A` array segment (OUTM +
OUTDATA), `srcBase` = array byte `0x2F22` = OUTDATA payload `0x400` (the
override taken when `ds:2082h == 0x23CD`, the overworld tile-buffer
base -- byte `0x479A`).  So the terrain sub-cells live in OUTDATA.
`readTileObject` clips partial edge tiles (`ds:2444h`-`ds:2452h`) with
`rtm_FE1B` / `rtm_FE14`.

Note: the `imul ds:2444h, 0x5F` in `resolveMoveTarget` is **not** a
record size -- `0x5F` (95) is the `OUTM` base-terrain grid width, used to
copy a 13x13 map window (`array[0x120 + Y*0x5F + X]`) for move logic.

## The object / creature sprites (payload 0x1400 onward)

**64 records x 124 bytes** in two banks: `0x1402`-`0x1AC9` (14 records =
7 pairs) and `0x1F5C`-`0x3793` (50 records = 25 pairs).  Each record is
a **stock MS-BASIC `PUT` GET-array**, same convention as `DUNMON`:

    dw 0x0028   -- X extent in BITS = 40  ->  20 px wide (CGA SCREEN 1, 2 bpp)
    dw 0x0014   -- Y extent = 20 rows
    100 bytes   -- 20 rows x ceil(40/8) = 5 bytes/row, linear 2 bpp,
                   MSB pair = leftmost pixel, colour 0 transparent
    20 bytes    -- zero padding to the 124-byte record stride

So each sprite is **20 x 20 px** -- one figure, NOT "two side-by-side
frames" (the earlier 40x12 reading was an artefact of a wrong 10 B/row
stride).  Records **pair up**: the even record is the colour image
(`basPutSprite` verb 0), the odd record right after it (+`0x7C` = 124 B)
is the `AND`-mask silhouette (verb 1) -- **32 pairs**.  `chainExec`'s
tail draws them for the special-travel events ("PEGASUS SETS YOU DOWN",
"AMBUSHED BY BANDITS!"): a walking man, an armoured warrior, a
sword-fighter, the pegasus, a centaur/mounted figure, wings, and a
menagerie of overworld monsters (bug, scorpion, octopus, spider, bat,
serpent, ...).  Static single-frame sprites -- no animation, no palette
cycling (see decoders/dunmon.py notes).
"""
import struct
import sys
import os

N_TILES = 256
REC = 4
BANK = 0x400
SPRITE_HDR = b"\x28\x00\x14\x00"
SPRITE_REC = 124
SPRITE_W, SPRITE_BPR, SPRITE_ROWS = 20, 5, 20
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
    """16-byte bank cell -> 8 rows of 8 palette indices (0..3).

    Field-interleaved: the 8 words go to screen scanlines 0,2,4,6 then
    1,3,5,7, so top-to-bottom read order is w0,w4,w1,w5,w2,w6,w3,w7.
    Within a word, memory is little-endian: the LOW byte holds the LEFT
    4 pixels, the high byte the right 4; leftmost pixel = bits 7-6 of
    its byte.  (Verified by rendering -- the group-of-4 coastline edge
    rotations only line up with this order.)"""
    o = BANK + n * 16
    if o + 16 > len(p):
        return [[0] * 8 for _ in range(8)]
    w = struct.unpack("<8H", p[o:o + 16])
    order = [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]
    out = []
    for wd in order:
        lo, hi = wd & 0xFF, wd >> 8
        out.append([(lo >> 6) & 3, (lo >> 4) & 3, (lo >> 2) & 3, lo & 3,
                    (hi >> 6) & 3, (hi >> 4) & 3, (hi >> 2) & 3, hi & 3])
    return out


def sprite_records(p):
    """Byte offsets of every 124-byte object/creature sprite record."""
    import re
    return [m.start() for m in re.finditer(re.escape(SPRITE_HDR), p)]


def sprite_pixels(p, off):
    """One 124-byte record -> 20 rows of 20 palette indices."""
    data = p[off + 4:off + 4 + SPRITE_BPR * SPRITE_ROWS]
    return [[(data[y * SPRITE_BPR + x // 4] >> (6 - 2 * (x % 4))) & 3
             for x in range(SPRITE_W)] for y in range(SPRITE_ROWS)]


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


def write_png(path, rows, scale=1):
    """rows = list of list of palette-index (0..3)."""
    import zlib
    h, w = len(rows), len(rows[0])
    raw = bytearray()
    for r in rows:
        line = bytearray()
        for v in r:
            line += bytes(PAL[v]) * scale
        for _ in range(scale):
            raw.append(0)
            raw += line
    def ch(t, d):
        return (struct.pack(">I", len(d)) + t + d
                + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF))
    open(path, "wb").write(
        b"\x89PNG\r\n\x1a\n"
        + ch(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0))
        + ch(b"IDAT", zlib.compress(bytes(raw), 9)) + ch(b"IEND", b""))


def sprite_sheet(p, path, scale=5, cols=8):
    """All sprite records: even = image, odd = AND-mask, side by side."""
    sr = sprite_records(p)
    gap = 1
    cw, cht = SPRITE_W + gap, SPRITE_ROWS + gap
    rows_n = (len(sr) + cols - 1) // cols
    grid = [[0] * (cw * cols) for _ in range(cht * rows_n)]
    for i, off in enumerate(sr):
        spr = sprite_pixels(p, off)
        ox, oy = (i % cols) * cw, (i // cols) * cht
        for y, row in enumerate(spr):
            for x, v in enumerate(row):
                grid[oy + y][ox + x] = v
    write_png(path, grid, scale)


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

    sr = sprite_records(p)
    strides = {sr[i + 1] - sr[i] for i in range(len(sr) - 1) if sr[i + 1] - sr[i] < 0x100}
    b1 = [o for o in sr if o < 0x1B00]
    b2 = [o for o in sr if o > 0x1B00]
    print("\nobject/creature sprites: %d records x 124 B (%d image/mask pairs),"
          " 20x20 px, strides %s" % (len(sr), len(sr) // 2, strides))
    print("  bank 1: 0x%X .. 0x%X  (%d recs)   bank 2: 0x%X .. 0x%X  (%d recs)"
          % (b1[0], b1[-1], len(b1), b2[0], b2[-1], len(b2)))

    if outdir:
        os.makedirs(outdir, exist_ok=True)
        tiles = [t for t, _ in recs]
        out = os.path.join(outdir, "outdata_terrain.png")
        cols = 16
        rn = (len(tiles) + cols - 1) // cols
        grid = [[0] * (18 * cols) for _ in range(18 * rn)]
        for i, T in enumerate(tiles):
            img = terrain_tile(p, T)
            ox, oy = (i % cols) * 18, (i // cols) * 18
            for y in range(16):
                for x in range(16):
                    grid[oy + y][ox + x] = img[y][x]
        write_png(out, grid, 6)
        print("wrote", out, "(%d assembled 16x16 terrain tiles)" % len(tiles))

        # the raw 146-cell sub-cell bank
        cols = 16
        rn = (146 + cols - 1) // cols
        grid = [[0] * (9 * cols) for _ in range(9 * rn)]
        for n in range(146):
            cell = sub_cell(p, n)
            ox, oy = (n % cols) * 9, (n // cols) * 9
            for y in range(8):
                for x in range(8):
                    grid[oy + y][ox + x] = cell[y][x]
        out = os.path.join(outdir, "outdata_subcells.png")
        write_png(out, grid, 7)
        print("wrote", out, "(146-cell 8x8 sub-cell bank, 0x400-0xD1F)")

        out = os.path.join(outdir, "outdata_sprites.png")
        sprite_sheet(p, out)
        print("wrote", out, "(64 records; even col = image, odd col = AND-mask)")


if __name__ == "__main__":
    main()

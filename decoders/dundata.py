#!/usr/bin/env python3
"""Decode `DUNDATA.BSV` -- the first-person dungeon-view wall / floor /
ceiling tile graphics for `DUN.EXE` (`bmDUNG`).  `MUSDATA.BSV` carries a
near-copy for the museum exhibit halls.

    python decoders/dundata.py [C:\\games\\lota\\DUNDATA.BSV] [outdir]

`loadDungeonData` `BLOAD`s the 5778-byte payload into `dungeonMapArray`
(`ds:1E2A`) at **byte offset 0x800** -- immediately after the 2048-byte
`DUNM<n>.BSV` tile map, in the same array.  Every offset word in DUNDATA
is an *array* offset (`0x800 + payloadOffset`), used raw.

## The renderer

`renderDungeonView` (bmDUNG) loops 5 depth bands.  Per band it calls
`drawViewWallBandNear` (front wall + ceiling + floor) and, if the view
isn't blocked, `drawViewFloorCeiling` twice (left + right side).  Those
funnel through `blitViewCell` -> `drawTileRun` (`rtm_FE2A`):

    drawTileRun(srcBase, srcSeg, videoOff, tileIdxList, count):
        for i in 0..count-1:
            b = tileIdxList[i]
            if b != 0xFF:                       # 0xFF = transparent
                copy the 16-byte bank cell at srcBase + b*16 to video
                at videoOff + i*2                # one 8-px column across
    blitViewCell(srcBase, tileIdxListPtr, slotIdx):
        (videoOff, packedDims) = arrayWord[slotIdx], arrayWord[slotIdx+1]
        for r in 0..nbands-1:                    # packedDims = (ncols<<8)|nbands
            drawTileRun(srcBase, seg, videoOff + r*0x140,
                        tileIdxListPtr + r*ncols, ncols)

`renderDungeonView` threads a word cursor (`[bp-14h]`, init `0x410` =
payload byte `0x20`) forward `0xA, 7, 7` words per band -- so the record
table is walked strictly in sequence.

## DUNDATA layout (payload 0x1692 = 5778 B)

    0x000-0x01F  header.  word[0] = 0x0E94 = the tile bank's array
                 offset (= payload 0x694); drawTileRun's srcBase.
    0x020-0x10F  the projection record table -- 15 records, per depth:
                   * wall-band record (10 words) = 3 triples
                     `(videoOff, packedDims, tileListPtr)` +1 pad word:
                     triple 0 = the CEILING strip (wide, e.g. 23x2 near
                     -> 5x0 far), triple 1 = the FLOOR strip, triple 2 =
                     the FRONT-WALL block (e.g. 17x13 near -> 3x5 far).
                   * left-side record (7 words) and right-side record
                     (7 words) = `(videoOff, packedDims, p0,p1,p2,p3,
                     padWord)`.  p0..p3 point at the 4 columns of the
                     strip table (deltas 0xCC); packedDims shrinks
                     3x15 -> 1x5 with depth.  `drawViewFloorCeiling`
                     picks p0 (col 0) when that side is a solid wall,
                     p1 (col 1) when it's an open passage.
    0x110-0x1BB  wall-band tile lists (the ceiling / floor / front-wall
                 index bytes; more of them live at 0x4EC-0x693).
    0x1BC-0x4EB  the strip table -- 4 columns x 0xCC bytes = 4 render
                 variants of the corridor side walls.  Within a column
                 the 10 side records' strips pack contiguously, each
                 `ncols*nbands` bytes (X = 0x1BC, +0x2D, +0x2D, +0x1E,
                 +0x1E, +0x10, +0x10, +6, +6, +5).
    0x4EC-0x693  more wall-band tile lists (front-wall blocks).
    0x694-0x1691 the 8x8 tile bitmap bank: 255 cells x 16 B, CGA 2 bpp,
                 8 words per cell -> screen rows 0,2,4,6,1,3,5,7 (so
                 read-back order is w0,w4,w1,w5,w2,w6,w3,w7).  Cell 0 is
                 blank.

Every tile list -- wall-band strip or side strip -- is a **flat
`ncols x nbands` row-major array of bank cell indices** (`0xFF` = skip)
fed straight to `drawTileRun`.  There is no marker / display-list layer;
bytes like `0x00`, `0x4B`-`0x56`, `0x70`, `0x7E`-`0x7F`, `0xC0`-`0xCC`
are just the near-black shadow / edge bank cells every surface uses
along its boundaries.

The blocked-view fallback re-uses the same records at other word
offsets, no new fields:

  * if a wall tile blocks the corridor at depth k, the near loop stops
    with the cursor on depth k's LEFT-side record.  For each side:
      - open side  -> `drawViewWallBandMid` draws that side wall from the
        record's `p2` (word 4) or `p3` (word 5) -- the darker strip-table
        columns, chosen on the blocking wall's `< 0x80` thickness;
      - solid side -> `drawViewFloorCeiling` + an `rtm_FE2B` fill.
  * if the corridor is open all the way, `drawViewWallBandFar` draws
    depth 4's FRONT-WALL triple (words 6/7/8 of the depth-4 wall-band
    record), tile list offset `+0xF`.
"""
import struct
import sys
import os

ARRAY_BASE = 0x800          # DUNDATA loads at dungeonMapArray byte 0x800
CURSOR0 = 0x410             # renderDungeonView's [bp-14h] init (word units)
BAND_STEPS = (0xA, 7, 7)    # cursor advance: wall-band, left side, right side
CELL = 16
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def load(path):
    d = open(path, "rb").read()
    return d[7:] if d[:1] == b"\xFD" else d


def w(p, payoff):
    return struct.unpack("<H", p[payoff:payoff + 2])[0]


def bank_base(p):
    return w(p, 0) - ARRAY_BASE


def a2p(off):
    """array offset -> payload offset"""
    return off - ARRAY_BASE


def records(p):
    """Walk the 15-record table exactly as renderDungeonView does."""
    cur = CURSOR0
    out = []
    for depth in range(5):
        po = cur * 2 - ARRAY_BASE
        wb = [w(p, po + 2 * k) for k in range(10)]
        triples = [(wb[i], wb[i + 1], wb[i + 2]) for i in (0, 3, 6)]
        out.append(("wallband", depth, dict(triples=triples, pad=wb[9])))
        cur += BAND_STEPS[0]
        for si, side in enumerate(("left", "right")):
            po = cur * 2 - ARRAY_BASE
            r = [w(p, po + 2 * k) for k in range(7)]
            out.append(("side", depth, dict(side=side, videoOff=r[0],
                        dims=r[1], quad=r[2:6], pad=r[6])))
            cur += BAND_STEPS[1 + si]
    return out


def cell_pixels(p, off):
    ws = struct.unpack("<8H", p[off:off + CELL])
    order = [ws[0], ws[4], ws[1], ws[5], ws[2], ws[6], ws[3], ws[7]]
    return [[(row >> (14 - 2 * x)) & 3 for x in range(8)] for row in order]


def tile_list(p, ptr, ncols, nbands):
    """The flat ncols x nbands cell-index grid a record points at."""
    base = a2p(ptr)
    n = ncols * max(nbands, 1)
    b = p[base:base + n]
    return [list(b[r * ncols:(r + 1) * ncols]) for r in range(max(nbands, 1))]


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
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\DUNDATA.BSV"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(outdir, exist_ok=True)
    p = load(path)
    BANK = bank_base(p)
    if not (0 < BANK < len(p)):
        sys.exit("word[0]=0x%X: not a DUNDATA-format file (MUSDATA differs)" % w(p, 0))
    ncell = (len(p) - BANK) // CELL
    print("payload %d B   bank @ payload 0x%X = %d cells" % (len(p), BANK, ncell))

    print("\nprojection record table (0x20-0x10F, 15 records):")
    for kind, depth, r in records(p):
        if kind == "wallband":
            print("  depth %d  wall-band:" % depth)
            for name, (vo, pd, tl) in zip(("ceiling", "floor  ", "front  "), r["triples"]):
                print("      %s  vOff=%04X  %2dx%-2d  tileList=%04X -> payload 0x%03X"
                      % (name, vo, pd >> 8, pd & 0xFF, tl, a2p(tl)))
        else:
            q = r["quad"]
            print("  depth %d  %-5s side  vOff=%04X  %dx%d  strip-quad %s (payload 0x%03X, +0xCC x3)"
                  % (depth, r["side"], r["videoOff"], r["dims"] >> 8, r["dims"] & 0xFF,
                     " ".join("%04X" % x for x in q), a2p(q[0])))

    # render the bank
    S, cols = 8, 16
    rowsN = (ncell + cols - 1) // cols
    cw = 8 * S + 1
    canvas = [[0] * (cw * cols) for _ in range(cw * rowsN)]
    for n in range(ncell):
        px = cell_pixels(p, BANK + n * CELL)
        ox, oy = (n % cols) * cw, (n // cols) * cw
        for y in range(8):
            for x in range(8):
                v = px[y][x]
                for dy in range(S):
                    for dx in range(S):
                        canvas[oy + y * S + dy][ox + x * S + dx] = v
    out = os.path.join(outdir, "dundata_bank.bmp")
    write_bmp(out, canvas)
    print("\nwrote", out, "(%d cells)" % ncell)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Decode `DUNDATA.BSV` -- the first-person dungeon-view wall / floor /
ceiling tile graphics for `DUN.EXE` (`bmDUNG`).  `MUSDATA.BSV` carries a
near-copy for the museum exhibit halls.

    python decoders/dundata.py [C:\\games\\lota\\DUNDATA.BSV] [outdir]

`loadDungeonData` `BLOAD`s the 5778-byte payload into `dungeonMapArray`
(`ds:1E2A`) at **byte offset 0x800** -- immediately after the 2048-byte
`DUNM<n>.BSV` tile map, in the same array.

## The renderer

`renderDungeonView` (bmDUNG) walks the view ray and, per depth band,
calls `drawViewWallBand{Near,Mid,Far}` / `drawViewFloorCeiling`, which
all funnel through **`blitViewCell`**, which calls **`drawTileRun`**
(`rtm_FE2A`, leglib `seg004:0x1fd3f`):

    drawTileRun(srcBankBase, srcSeg, videoOff, tileIdxList, count):
        for i in 0..count-1:
            b = tileIdxList[i]
            if b != 0xFF:                         # 0xFF = transparent
                copy the 16-byte cell at srcBankBase + b*16
                to video at videoOff + i*2         # one 8-px column across
        # the copy (sub_1FED8) writes the 8 source words to CGA rows
        # 0,2,4,6 (even field) then 1,3,5,7 (odd field) -- so the cell is
        # stored as 8 *linear* scanlines; the field split is on write.

`blitViewCell(srcBankBase, tileIdxListPtr, slotIdx)` reads a
`(videoOff, packedDims)` word pair from `dungeonMapArray[slotIdx]`
(word-indexed), with `packedDims = (ncols << 8) | nbands`, then loops
`nbands` rows: row r is `drawTileRun(srcBankBase, seg,
videoOff + r*0x140, tileIdxListPtr + r*ncols, ncols)`.

## DUNDATA layout (payload 0x1692 = 5778 B)

    0x000-0x03F  header.  word[0] = 0x0E94 = the *array* byte offset of
                 the tile bitmap bank (DUN.EXE reads it as the `srcBase`
                 for drawTileRun); DUNDATA loads at array byte 0x800, so
                 that is payload offset 0x694.  Bytes 0x20-0x3F: a
                 pre-table lookup row.
    0x040-~0x110 projection record table: ~13 variable-length records,
                 ~3 per view depth (left / front / right wall).  Each:
                 `dw screenBase` (0x28 / 0x50 / 0x78 / 0xA0 / 0xC8 =
                 0x28*(depth+1)) ; `dw videoOff` ; `dw packedDims` ;
                 then a run of pointer words.  packedDims = (ncols<<8) |
                 nbands -- both shrink with depth (0x030F = 3x15 near ->
                 0x0105 = 1x5 far).  Side-wall records carry 4 pointers
                 at a 0xCC stride; front-wall records carry 7.
    ~0x110-0x693 the tile-index byte lists.  Bank cell indices
                 (0x00-0xFE); 0x08-0x14 = wall face + floor/ceiling ramp,
                 0x16-0x19 = junction edges, 0x1E = fill, >=0x40 =
                 corner / trim / shadow cells and display-list markers.
                 The `0x1E8` sub-region is a **4-column table**, each
                 column `0xCC` bytes = one **lighting/distance level** of
                 the wall geometry (col 0 fully lit, col 1 shadowed,
                 col 2 lit, col 3 darkest / uses the `0xB0`-`0xCC` cell
                 set).  Within a column, the **9 side-wall strips** are
                 packed **contiguously from byte 1** (byte 0 of each
                 `0xCC` block is unused -- the "recurring 4D 41 28" is
                 that pad byte plus the first wall's top corner cells).
                 A side-wall projection record carries the pointer quad
                 `X / X+0xCC / X+0x198 / X+0x264` -- the same wall at the
                 4 levels.  `X` advances by exactly `ncols*nbands` per
                 wall (0x1E9, +0x2D, +0x1E, +0x1E, +0x10, ...); dims
                 shrink `3x15` -> `1x5` with depth.  **A strip is a flat
                 `ncols x nbands` row-major array of bank cell indices**
                 (0xFF = skip) fed straight to `drawTileRun` -- no
                 markers.  The bytes that look like delimiters
                 (`0x00`, `0x4B`-`0x56`, `0x70`, `0x7E`-`0x7F`,
                 `0xC0`-`0xCC`) are just the near-black shadow/edge bank
                 cells that every wall uses along its floor line, ceiling
                 line and near vertical edge.
    0x694-0x1691 the 8x8 tile bitmap bank: 255 cells x 16 B, CGA 2 bpp,
                 8 linear scanline-words per cell (cell 0 = blank).

Open: the cursor threading in renderDungeonView (`[bp-14h]` init 0x410
words, stepped 0xA/7/7 per band) that maps each record to an on-screen
wall, and the pointer-list fields inside a record.
"""
import struct
import sys
import os

ARRAY_BASE = 0x800   # DUNDATA loads at dungeonMapArray byte 0x800
CELL = 16
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def load(path):
    d = open(path, "rb").read()
    return d[7:] if d[:1] == b"\xFD" else d


def bank_base(p):
    """word[0] is the tile bank's *array* byte offset; convert to payload."""
    return struct.unpack("<H", p[0:2])[0] - ARRAY_BASE


def cell_pixels(p, off):
    """16-byte bank cell -> 8 rows of 8 palette indices.

    sub_1FED8 reads the 8 words sequentially and writes them to CGA rows
    0,2,4,6 (even field) then 1,3,5,7 (odd field), so the on-screen row
    order is w0,w4,w1,w5,w2,w6,w3,w7."""
    w = struct.unpack("<8H", p[off:off + CELL])
    order = [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]
    return [[(row >> (14 - 2 * x)) & 3 for x in range(8)] for row in order]


# The 9 side-wall strips, packed contiguously from byte 1 of each column.
# (X, ncols, nbands) -- X is the offset within column 0.
SIDE_WALLS = [(0x1E9, 3, 15), (0x216, 3, 10), (0x234, 3, 10),
              (0x252, 2, 8), (0x262, 2, 8), (0x272, 1, 6),
              (0x278, 1, 6), (0x27E, 1, 5), (0x283, 1, 5)]
STRIP_TABLE = 0x1E8
STRIP_COL = 0xCC


def wall_strip(p, wall, col):
    """The (ncols x nbands) cell-index grid for side wall `wall` at
    lighting/distance level `col` (0-3)."""
    X, nc, nb = SIDE_WALLS[wall]
    off = X + col * STRIP_COL
    return [[p[off + r * nc + c] for c in range(nc)] for r in range(nb)]


def records(p):
    """Walk the projection record table.  A record starts at a
    `dw 0x0028*(depth+1)` marker and runs to the next marker; each record
    is `screenBase, videoOff, packedDims, <pointer words>`.  Stops when a
    record's dims stop looking like `(ncols<<8)|nbands`."""
    markers = {0x28, 0x50, 0x78, 0xA0, 0xC8}
    W = struct.unpack("<%dH" % (len(p) // 2), p[:len(p) // 2 * 2])
    starts = [i for i in range(0x10, 0x100) if W[i] in markers]
    recs = []
    for a, b in zip(starts, starts[1:] + [starts[-1] + 16]):
        r = [W[j] for j in range(a, min(b, len(W)))]
        if len(r) < 3:
            break
        ncols, nbands = r[2] >> 8, r[2] & 0xFF
        if not (1 <= ncols <= 0x20 and nbands <= 0x20 and r[1] < len(p)):
            break
        recs.append((a, r))
    return recs


def write_bmp(path, rows, scale=1):
    h = len(rows)
    w = len(rows[0]) if h else 0
    W_, H_ = w * scale, h * scale
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
        sys.exit("word[0]=0x%X: not a DUNDATA-format file (MUSDATA differs)"
                 % struct.unpack("<H", p[0:2])[0])
    ncell = (len(p) - BANK) // CELL
    print("payload %d B  word[0]=0x%X -> bank at payload 0x%X  = %d cells of 16 B"
          % (len(p), struct.unpack("<H", p[0:2])[0], BANK, ncell))

    print("\nprojection record table (screenBase / videoOff / packedDims / pointers):")
    for a, r in records(p):
        if len(r) < 3 or r[0] not in (0x28, 0x50, 0x78, 0xA0, 0xC8):
            break
        dims = r[2]
        print("  @w0x%03X  screenBase=0x%02X  videoOff=0x%04X  dims=0x%04X (ncols=%d nbands=%d)  ptrs: %s"
              % (a, r[0], r[1], dims, dims >> 8, dims & 0xFF,
                 " ".join("%04X" % x for x in r[3:])))

    print("\n0x1E8 strip table -- 4 columns x 0x%X B (lighting/distance levels)." % STRIP_COL)
    print("9 side-wall strips packed contiguously from byte 1 of each column;")
    print("each strip = flat ncols x nbands bank-cell-index grid (0xFF = skip):")
    for w, (X, nc, nb) in enumerate(SIDE_WALLS):
        b = bytes(p[X + r * nc + c] for r in range(nb) for c in range(nc))
        print("  wall %d  X=0x%03X  %dx%d = %2d cells   %s" % (w, X, nc, nb, nc * nb, b.hex()))

    # render the whole bank, 16 cells/row
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

#!/usr/bin/env python3
"""Decode `DUNOBJ.BSV` -- the dungeon-view object table + per-depth
sprite-mask cells (`DUN.EXE` / `bmDUNG`).  `MUSOBJ.BSV` / `OUTOBJ.BSV`
share the container.

    python decoders/dunobj.py [C:\\games\\lota\\DUNOBJ.BSV] [outdir]

`loadDungeonData` `BLOAD`s the 9344-byte payload into `spriteBank`
(`ds:1E58`) at **offset 0** -- the BSAVE header's `0x0DB6` is ignored --
so every file offset here is also its `spriteBank` offset, with no
relocation.  `DUNMONA/B.BSV` then loads immediately after, at word
`0x1240` (= byte `0x2480` = this payload's length).

Layout (real data ends at 0x15A1):

    0x000-0x045  dw 0x0005, then 7 x 10-byte object records
                 `dw 0x0110 ; dw spriteWord ; dw 0 ; dw spriteWord ; dw K`
    0x046-0x18F  5 per-depth mask descriptors, one every 0x28 words at
                 P_idx in {0x23,0x4B,0x73,0x9B,0xC3}, each
                 `dw 0x0110 ; dw endWord ; dw count ; dw startWord ; dw K`
                 (endWord == startWord + count*2)
    0x190-0x1A3  DUNMON bank table: 6 words 0x1240 + k*0x49A
    ~0x400-0x8F1 region B: per-depth (videoDest, maskSrc) word-pair lists
    0x8F2-0x1211 object/decoration sprite bitmaps (consumer still open)
    0x1212-0x15A1 region C: 57 contiguous 16-byte AND-mask cells
    0x15A2-end   zero padding

`drawViewSprite`, per view-depth band P (0-4), reads the descriptor at
`spriteBank[P_idx]`: word `+3` = mask-cell count, word `+4` = start word
of that depth's region-B list.  It then walks `count` `(videoDest,
maskSrc)` pairs, calling `andSpriteMaskCell(src=maskSrc, seg, dest=
videoDest)` for each.  `maskSrc` is `0x1212 + 16*n` -- a direct byte
offset to a region-C cell.

A region-C cell is one **field-interleaved 8x8, 2bpp AND stencil**: 8
words, screen-scanline order 0,2,4,6,1,3,5,7; bits 15-14 = leftmost
pixel.  `and es:[di], word` per scanline: a pixel-pair of `11` keeps the
video pixel, `00` forces it to colour 0.  The cells are dither patterns;
composited per depth they make a rectangular aperture with a dithered
top edge that shrinks with distance -- the lit niche a monster is drawn
into.
"""
import struct
import sys
import os

P_IDX = {0: 0x23, 1: 0x4B, 2: 0x73, 3: 0x9B, 4: 0xC3}
REGION_C = 0x1212
CELL = 16
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def load(path):
    d = open(path, "rb").read()
    if d[:1] == b"\xFD":
        d = d[7:]
    return d


def words(p):
    return struct.unpack("<%dH" % (len(p) // 2), p[:len(p) // 2 * 2])


def object_records(W):
    out = []
    i = 1  # after `dw 0x0005`
    while W[i] == 0x0110 and W[i + 2] == 0:      # B field 0 -> object record
        out.append(dict(sprite=W[i + 1], K=W[i + 4]))
        i += 5
    return out


def mask_descriptors(W):
    out = {}
    for P, pi in P_IDX.items():
        tag, end, count, start, K = W[pi + 1], W[pi + 2], W[pi + 3], W[pi + 4], W[pi + 5]
        assert tag == 0x0110 and start + count * 2 == end, (P, hex(pi))
        out[P] = dict(count=count, start=start, end=end, K=K)
    return out


def pair_list(W, start, count):
    return [(W[start + i * 2], W[start + i * 2 + 1]) for i in range(count)]


def cell_scanlines(p, off):
    """16-byte region-C cell -> 8 screen scanlines (each a 16-bit word)."""
    w = struct.unpack("<8H", p[off:off + CELL])
    return [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]


def cell_pixels(p, off):
    return [[(sl >> (14 - 2 * x)) & 3 for x in range(8)] for sl in cell_scanlines(p, off)]


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


def region_c_count(p):
    end = len(p)
    while end > REGION_C and p[end - 1] == 0:
        end -= 1
    return (end - REGION_C + CELL - 1) // CELL


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\DUNOBJ.BSV"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(outdir, exist_ok=True)
    p = load(path)
    W = words(p)

    print("object records:")
    for i, r in enumerate(object_records(W)):
        print("  %d: spriteWord=%#06x  K=%d" % (i, r["sprite"], r["K"]))

    print("\nmask descriptors + region-B lists:")
    md = mask_descriptors(W)
    used = set()
    for P in range(5):
        d = md[P]
        pl = pair_list(W, d["start"], d["count"])
        srcs = sorted({ms for _, ms in pl})
        used |= set(srcs)
        print("  P%d  count=%2d  list w%#05x-w%#05x  cells %#06x..%#06x (%d unique)"
              % (P, d["count"], d["start"], d["end"], srcs[0], srcs[-1], len(srcs)))

    ncell = region_c_count(p)
    print("\nregion C: %#06x .. %#06x  = %d cells of %d bytes  (%d referenced)"
          % (REGION_C, REGION_C + ncell * CELL, ncell, CELL, len(used)))

    # montage of the whole region-C pool, 8 cells per row, x6
    S, cols = 6, 8
    rowsN = (ncell + cols - 1) // cols
    cw = ch = 8 * S + 1
    canvas = [[0] * (cw * cols) for _ in range(ch * rowsN)]
    for n in range(ncell):
        px = cell_pixels(p, REGION_C + n * CELL)
        ox, oy = (n % cols) * cw, (n // cols) * ch
        for y in range(8):
            for x in range(8):
                v = px[y][x]
                for dy in range(S):
                    for dx in range(S):
                        canvas[oy + y * S + dy][ox + x * S + dx] = v
    out = os.path.join(outdir, "dunobj_regionC_pool.bmp")
    write_bmp(out, canvas)
    print("wrote", out)

    # per-depth composited aperture (white = kept by the AND)
    for P in range(5):
        d = md[P]
        pl = pair_list(W, d["start"], d["count"])
        Hh, Ww, oy0, ox0 = 120, 180, 30, 120
        cv = [[1] * Ww for _ in range(Hh)]
        for vd, ms in pl:
            row = (vd // 0x50) * 2
            col = (vd % 0x50) * 4
            sl = cell_scanlines(p, ms)
            for r in range(8):
                for x in range(8):
                    bit = (sl[r] >> (14 - 2 * x)) & 3
                    yy, xx = row + r - oy0, col + x - ox0
                    if 0 <= yy < Hh and 0 <= xx < Ww:
                        cv[yy][xx] = 3 if (cv[yy][xx] and bit == 3) else 0
        write_bmp(os.path.join(outdir, "dunobj_aperture_P%d.bmp" % P), cv, scale=3)
    print("wrote dunobj_aperture_P0..P4.bmp")


if __name__ == "__main__":
    main()

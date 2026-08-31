#!/usr/bin/env python3
"""Decode `CEL*.BSV` / `DIS*.BSV` -- the "AGAINST ALL ODDS!" endgame
cinematic frames (`CELDRV.EXE`) and the museum exhibit illustration
screens (`MUS.EXE`).  Both use the identical container.

    python decoders/cel_image.py [C:\\games\\lota\\CEL0.BSV] [outdir]

## Container

    8-word header:
        [0] id / bank marker
        [1] 0x0010  (const)
        [2] W  (pixel width -- CEL0 / DIS9 = 0x0110 = 272)
        [3] H  (pixel height -- 0x0078 = 120)
        [4] 0x0020  (const)
        [5] 0x000A  (const)
        [6] X  (0x10 or 0x30 -- a mode / left-margin flag)
        [7] 0x0220  -- the strip-region base offset

    0x0E..0xDF : mostly zero (a few files put a small extra table at 0x20)
    0xE0 .. header[7] : the CELL TABLE -- up to 80 4-byte entries
        `dw videoDest ; dw stripPtr`.  It ends exactly at header[7].
        videoDest is a CGA even-field byte offset: bands step 0x140
        (= 8 screen scanlines), columns step +2 (= 8 px).  So the frame
        is a 10-col x 8-row grid of 8x8 cells (fewer for small frames).
        `CELDRV` relocates `stripPtr` by the BLOAD word-offset.
    header[7] .. EOF : the STRIP DATA -- 16-byte **field-interleaved**
        8x8 CGA cells (word order -> screen rows 0,2,4,6,1,3,5,7), packed
        16-byte-aligned.  A cell table entry with a longer gap to the
        next distinct `stripPtr` owns a horizontal run of that many
        cells (drawn at videoDest, +2 per cell).

**There is no RLE.**  The "compression" is that a frame stores only its
changed 8x8 cells and de-duplicates identical ones (several table
entries can share one `stripPtr`).  `CEL0..CEL3` + `DIS9` are 5
animation frames painted in sequence over the same region.
"""
import struct
import sys
import os

HEADER_WORDS = 8
TABLE = 0xE0
CELL = 16
PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def load(path):
    d = open(path, "rb").read()
    return d[7:] if d[:1] == b"\xFD" else d


def header(p):
    h = struct.unpack("<8H", p[:16])
    return dict(id=h[0], w=h[2], h=h[3], mode=h[6], strip_base=h[7])


def cell_table(p):
    """[(videoDest, stripPtr), ...] -- entries from 0xE0 to strip_base."""
    base = header(p)["strip_base"]
    out = []
    for a in range(TABLE, base, 4):
        vd, sp = struct.unpack("<HH", p[a:a + 4])
        out.append((vd, sp))
    return out


def strip_len(sorted_ptrs, sp, end):
    i = sorted_ptrs.index(sp)
    return (sorted_ptrs[i + 1] if i + 1 < len(sorted_ptrs) else end) - sp


def cell_rows(p, off):
    """16-byte field-interleaved cell -> 8 rows of 8 palette indices."""
    chunk = p[off:off + CELL].ljust(CELL, b"\0")
    w = struct.unpack("<8H", chunk)
    order = [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]
    return [[(row >> (14 - 2 * x)) & 3 for x in range(8)] for row in order]


def paint(p, screen):
    """Paint one CEL/DIS frame onto `screen` (200 rows x 320 CGA px)."""
    tbl = cell_table(p)
    ptrs = sorted(set(sp for _, sp in tbl))
    for vd, sp in tbl:
        n = strip_len(ptrs, sp, len(p)) // CELL
        for c in range(n):
            rows = cell_rows(p, sp + c * CELL)
            off = vd + c * 2
            row0, col0 = (off // 0x50) * 2, (off % 0x50) * 4
            for r in range(8):
                for x in range(8):
                    y, xx = row0 + r, col0 + x
                    if 0 <= y < 200 and 0 <= xx < 320:
                        screen[y][xx] = rows[r][x]


def write_bmp(path, rows, scale=3):
    h = len(rows)
    ww = len(rows[0])
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
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\CEL0.BSV"
    outdir = sys.argv[2] if len(sys.argv) > 2 else None
    p = load(path)
    hd = header(p)
    tbl = cell_table(p)
    ptrs = sorted(set(sp for _, sp in tbl))
    print("%s  payload %d B" % (os.path.basename(path), len(p)))
    print("  header: id=%#x  %dx%d  mode=%#x  strip_base=%#x"
          % (hd["id"], hd["w"], hd["h"], hd["mode"], hd["strip_base"]))
    print("  cell table: %d entries (0xE0-%#x), %d distinct strips, %d strip bytes"
          % (len(tbl), hd["strip_base"], len(ptrs), len(p) - hd["strip_base"]))
    for vd, sp in tbl[:8]:
        print("    videoDest=%04X  stripPtr=%04X  len=%#x"
              % (vd, sp, strip_len(ptrs, sp, len(p))))

    if outdir:
        os.makedirs(outdir, exist_ok=True)
        scr = [[0] * 320 for _ in range(200)]
        paint(p, scr)
        out = os.path.join(outdir, os.path.splitext(os.path.basename(path))[0] + ".bmp")
        write_bmp(out, scr)
        print("  wrote", out)


if __name__ == "__main__":
    main()

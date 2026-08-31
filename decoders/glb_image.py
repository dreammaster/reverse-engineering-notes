#!/usr/bin/env python3
"""Decode a `.GLB` tile sheet + `.GMP` cell map into a CGA image and render
it to a 24-bit BMP.  Generalised from `decoders/title_screen.py` and
verified against TITLE + SDMAP -- see docs/file-formats.md.

    python decoders/glb_image.py NAME [C:\\games\\lota] [out.bmp]

  NAME is the stem, e.g. TITLE or SDMAP (loads NAME.GLB + NAME.GMP).
  Writes NAME.bmp (1x) and NAME_2x.bmp.

Format
------
Both files are Microsoft BASIC BSAVE images: [FD][seg:2][off:2][len:2][payload].

`.GLB` payload (the tile sheet):
    bytes[0..9]   5 header words.  word[0] = 0x000A (this header's size).
                  The remaining four (TITLE {8,1,1,1}, SDMAP {4,1,1,0}) look
                  like BASIC DIM bounds and are NOT needed to render -- in
                  particular word[1] is *not* the tile width.
    bytes[10..]   a flat array of 8x8 CGA tiles, 16 bytes each, 2 bpp /
                  4 colours.  Stored FIELD-INTERLEAVED: the 8 words are
                  scanlines 0,2,4,6,1,3,5,7 in that file order.  Within a
                  word, byte 0 = the left 4 px, top bit-pair = leftmost px.

`.GMP` payload (the cell map):
    bytes[0..5]   3 header words {0x1A, 0x11, 0x1A}  (0x1A = this header size)
    word[3]       ROWS  (tile rows == words per column; column-major)
    word[4]       COLS  (tile columns; COLS*8 == screen width, +pad column)
    bytes[10..]   "<name>" as wide chars (the BASIC source var), zero-padded
    bytes[0x1A..] the cell array: COLS columns x ROWS words, COLUMN-MAJOR.
                  Cell word W selects tile  W // 8  (the blitter does
                  `src = tiledata + W*2`, and a tile is 16 bytes; every W
                  seen so far is a multiple of 8).  The last (COLS-1) column
                  is padding -- COLS*8 overruns 320.

Screen: CGA 320x200 mode 4, palette 1 (0 black / 1 cyan / 2 magenta /
3 white); even scanlines at B800:0000, odd at B800:2000.
"""
import os
import struct
import sys

PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]

TILE_BYTES = 16       # 8x8 px, 2 bpp
SCREEN_W = 320


def read_bsave(path):
    b = open(path, "rb").read()
    if b[0] != 0xFD:
        raise ValueError(f"{path}: not a BSAVE image")
    seg, off, ln = struct.unpack_from("<HHH", b, 1)
    return b[7:7 + ln]


def decode(name, games_dir):
    glb = read_bsave(os.path.join(games_dir, name + ".GLB"))
    gmp = read_bsave(os.path.join(games_dir, name + ".GMP"))

    gh = struct.unpack_from("<5H", glb, 0)
    mh = struct.unpack_from("<5H", gmp, 0)
    rows, cols = mh[3], mh[4]
    tiles = glb[0x0A:]
    cells = gmp[0x1A:]
    print(f"{name}: GLB hdr {gh}  GMP hdr {mh}")
    print(f"  {len(tiles) // TILE_BYTES} tiles;  map = {cols} cols x {rows} rows "
          f"(column-major), {len(cells) // 2} cell words")

    W, H = min(cols * 8, SCREEN_W), rows * 8
    scr = bytearray(W * H)
    for col in range(cols):
        for row in range(rows):
            o = (col * rows + row) * 2
            if o + 1 >= len(cells):
                continue
            word = cells[o] | (cells[o + 1] << 8)
            src = word * 2                     # byte offset into the tile data
            for si in range(8):               # 8 file scanlines
                # first half -> even screen rows, second half -> odd
                r = (si * 2) if si < 4 else ((si - 4) * 2 + 1)
                y = row * 8 + r
                if y >= H:
                    continue
                b0 = tiles[src + si * 2] if src + si * 2 < len(tiles) else 0
                b1 = tiles[src + si * 2 + 1] if src + si * 2 + 1 < len(tiles) else 0
                for p in range(8):
                    x = col * 8 + p
                    if x >= W:
                        break
                    byte = b0 if p < 4 else b1
                    scr[y * W + x] = (byte >> (6 - (p % 4) * 2)) & 3
    return scr, W, H


def write_bmp(path, buf, w, h, pal, scale=1):
    if scale > 1:
        big = bytearray(w * scale * h * scale)
        for y in range(h):
            for x in range(w):
                v = buf[y * w + x]
                for dy in range(scale):
                    rb = (y * scale + dy) * w * scale
                    for dx in range(scale):
                        big[rb + x * scale + dx] = v
        buf, w, h = big, w * scale, h * scale
    rb = (w * 3 + 3) & ~3
    pad = rb - w * 3
    size = rb * h
    with open(path, "wb") as f:
        f.write(b"BM" + struct.pack("<IHHI", 14 + 40 + size, 0, 0, 14 + 40))
        f.write(struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, size, 2835, 2835, 0, 0))
        for y in range(h - 1, -1, -1):
            for x in range(w):
                r, g, b = pal[buf[y * w + x]]
                f.write(bytes((b, g, r)))
            f.write(b"\0" * pad)


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "TITLE"
    games = sys.argv[2] if len(sys.argv) > 2 else r"C:\games\lota"
    out = sys.argv[3] if len(sys.argv) > 3 else name.lower() + ".bmp"
    scr, w, h = decode(name, games)
    write_bmp(out, scr, w, h, PAL, scale=1)
    write_bmp(out.replace(".bmp", "_2x.bmp"), scr, w, h, PAL, scale=2)
    print(f"wrote {out} ({w}x{h}) and _2x")


if __name__ == "__main__":
    main()

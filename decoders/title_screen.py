#!/usr/bin/env python3
"""Decode TITLE.GLB + TITLE.GMP -> the menu title screen, and render it to a
24-bit BMP.

    python decoders/title_screen.py [C:\\games\\lota] [out.bmp]

Format (reverse-engineered from menu.exe seg001 -- loadTitleImage /
blitCharCell; see docs/file-formats.md):

  Both files are Microsoft BASIC BSAVE images: [FD][seg:2][off:2][len:2][payload].

  TITLE.GLB  (8209 b) -- the tile sheet.
    payload[0x00..0x09]  header words {0x000A, 0x0008, 1, 1, 1}
    payload[0x0A..]      a flat array of 512 16-byte tiles.  Each tile is an
                         8x8 CGA cell, 2 bpp (4 colours), stored
                         *field-interleaved*: words 0-3 = scanlines 0,2,4,6,
                         words 4-7 = scanlines 1,3,5,7.  Within a word, byte 0
                         is the left 4 px, MSB pair leftmost.

  TITLE.GMP  (2111 b) -- the 40x25 cell map.
    payload[0x00..0x19]  header: 5 words {0x1A, 0x11, 0x1A, 0x1A, 0x28} then
                         "title" as wide chars (the source variable name),
                         then zero padding.
    payload[0x1A..]      the cell array: 40 columns x 26 words, COLUMN-MAJOR
                         (25 rows used per column, 1 word padding).  Cell word
                         W selects tile  W // 8   (the blitter does
                         `src = tiledata + W*2`).

  Screen: CGA 320x200 mode 4, palette 1 (0 black / 1 cyan / 2 magenta /
  3 white); even scanlines at B800:0000, odd at B800:2000.
"""
import os
import struct
import sys

PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]
W, H = 320, 200


def read_bsave(path):
    b = open(path, "rb").read()
    if b[0] != 0xFD:
        raise ValueError(f"{path}: not a BSAVE image")
    seg, off, ln = struct.unpack_from("<HHH", b, 1)
    return seg, off, b[7:7 + ln]


def render(games_dir):
    _, _, glb = read_bsave(os.path.join(games_dir, "TITLE.GLB"))
    _, _, gmp = read_bsave(os.path.join(games_dir, "TITLE.GMP"))
    tiles = glb[0x0A:]
    cells = gmp[0x1A:]

    scr = bytearray(W * H)
    for col in range(40):
        for row in range(25):
            o = (col * 26 + row) * 2
            if o + 1 >= len(cells):
                continue
            word = cells[o] | (cells[o + 1] << 8)
            src = word * 2
            for wi in range(8):
                b0, b1 = tiles[src + wi * 2], tiles[src + wi * 2 + 1]
                y = row * 8 + (wi * 2 if wi < 4 else (wi - 4) * 2 + 1)
                for p in range(8):
                    byte = b0 if p < 4 else b1
                    scr[y * W + col * 8 + p] = (byte >> (6 - (p % 4) * 2)) & 3
    return scr


def write_bmp(path, buf, w, h, pal):
    row = (w * 3 + 3) & ~3
    pad = row - w * 3
    size = row * h
    with open(path, "wb") as f:
        f.write(b"BM" + struct.pack("<IHHI", 14 + 40 + size, 0, 0, 14 + 40))
        f.write(struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, size, 2835, 2835, 0, 0))
        for y in range(h - 1, -1, -1):
            for x in range(w):
                r, g, b = pal[buf[y * w + x]]
                f.write(bytes((b, g, r)))
            f.write(b"\0" * pad)


def main():
    games = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota"
    out = sys.argv[2] if len(sys.argv) > 2 else "title.bmp"
    scr = render(games)
    write_bmp(out, scr, W, H, PAL)
    # 2x for viewing
    big = bytearray(W * 2 * H * 2)
    for y in range(H):
        for x in range(W):
            v = scr[y * W + x]
            for dy in range(2):
                for dx in range(2):
                    big[(y * 2 + dy) * W * 2 + x * 2 + dx] = v
    write_bmp(out.replace(".bmp", "_2x.bmp"), big, W * 2, H * 2, PAL)
    print(f"wrote {out} (320x200) and {out.replace('.bmp', '_2x.bmp')} (640x400)")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Puts screenshots into one PNG for looking at (no PIL here): montage.py out.png cols scale file1 file2 ... (scale = 1 or 2: every 2nd pixel is taken)."""
import struct
import sys
import zlib
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pngread import read_png


def write_png(path, w, h, rows):
    raw = b''.join(b'\0' + bytes(v for px in r for v in px) for r in rows)

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


out, cols, step = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
files = sys.argv[4:]
imgs = [read_png(f) for f in files]
cw, ch = 320 // step, 200 // step
rows_n = (len(imgs) + cols - 1) // cols
H = rows_n * (ch + 1)
W = cols * (cw + 1)
canvas = [[(40, 40, 40)] * W for _ in range(H)]
for i, (w, h, rows) in enumerate(imgs):
    ox, oy = (i % cols) * (cw + 1), (i // cols) * (ch + 1)
    for y in range(ch):
        for x in range(cw):
            canvas[oy + y][ox + x] = rows[y * step][x * step]
write_png(out, W, H, canvas)
print(W, H)

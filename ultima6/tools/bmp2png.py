#!/usr/bin/env python3
"""Turns the 8-bit BMP that the ScummVM test hook writes (ultima6_shot) into a PNG, scaled up: bmp2png.py in.bmp out.png [scale]."""
import struct
import sys
import zlib


def main():
    src, dst = sys.argv[1], sys.argv[2]
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    d = open(src, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    pal = [(d[54 + i * 4 + 2], d[54 + i * 4 + 1], d[54 + i * 4]) for i in range(256)]
    rows = []
    for y in range(h):
        line = d[off + (h - 1 - y) * w: off + (h - y) * w]
        px = bytearray()
        for c in line:
            px += bytes(pal[c]) * scale
        for _ in range(scale):
            rows.append(b'\x00' + bytes(px))
    raw = b''.join(rows)

    def chunk(t, b):
        c = struct.pack('>I', len(b)) + t + b
        return c + struct.pack('>I', zlib.crc32(t + b) & 0xFFFFFFFF)

    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w * scale, h * scale, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
    open(dst, 'wb').write(png)


main()

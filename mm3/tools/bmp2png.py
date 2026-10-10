#!/usr/bin/env python3
"""bmp2png.py IN.bmp OUT.png [SCALE] -- convert a 24/32-bit BMP (from mm3game --shot) to PNG without extra packages."""
import struct, sys, zlib
d = open(sys.argv[1], 'rb').read()
sc = int(sys.argv[3]) if len(sys.argv) > 3 else 1
off = struct.unpack_from('<I', d, 10)[0]
w, h = struct.unpack_from('<ii', d, 18)
bpp = struct.unpack_from('<H', d, 28)[0]
bb = bpp // 8
rowb = (w * bb + 3) & ~3
raw = bytearray()
for y in range(abs(h)):
    sy = abs(h) - 1 - y if h > 0 else y
    row = d[off + sy * rowb: off + sy * rowb + w * bb]
    line = bytearray()
    for x in range(w):
        px = bytes((row[x * bb + 2], row[x * bb + 1], row[x * bb])) * sc
        line += px
    for _ in range(sc):
        raw += b'\0' + line
def chunk(t, b):
    c = struct.pack('>I', len(b)) + t + b
    return c + struct.pack('>I', zlib.crc32(t + b) & 0xffffffff)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w * sc, abs(h) * sc, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes(raw))) + chunk(b'IEND', b'')
open(sys.argv[2], 'wb').write(png)

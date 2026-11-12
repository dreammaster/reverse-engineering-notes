#!/usr/bin/env python3
"""Converts an IFF PBM picture (the *.PAL files of the spin-offs, e.g. MD_TITLE.PAL) to a PNG and prints its chunks.

    iff2png.py md/game/MARTIAN/MD_TITLE.PAL out.png
"""
import struct
import sys
import zlib


def png(path, w, h, rgb):
    raw = b''.join(b'\0' + bytes(rgb[y * w * 3:(y + 1) * w * 3]) for y in range(h))

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)

    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def main():
    d = open(sys.argv[1], 'rb').read()
    assert d[:4] == b'FORM' and d[8:12] == b'PBM '
    pos = 12
    w = h = 0
    comp = 0
    pal = None
    body = None
    while pos + 8 <= len(d):
        tag = d[pos:pos + 4]
        size = struct.unpack('>I', d[pos + 4:pos + 8])[0]
        data = d[pos + 8:pos + 8 + size]
        print(tag, size)
        if tag == b'BMHD':
            w, h = struct.unpack('>HH', data[:4])
            comp = data[10]
            print('  bmhd', w, h, 'compression', comp, 'planes', data[8])
        elif tag == b'CMAP':
            pal = data
        elif tag == b'BODY':
            body = data
        pos += 8 + size + (size & 1)
    if body is None:
        return
    pix = bytearray()
    if comp == 1:
        i = 0
        while i < len(body) and len(pix) < w * h:
            n = body[i]
            i += 1
            if n < 128:
                pix += body[i:i + n + 1]
                i += n + 1
            elif n > 128:
                pix += bytes([body[i]]) * (257 - n)
                i += 1
    else:
        pix = bytearray(body)
    rgb = bytearray()
    for y in range(h):
        for x in range(w):
            c = pix[y * w + x] if y * w + x < len(pix) else 0
            if pal:
                rgb += bytes([pal[c * 3], pal[c * 3 + 1], pal[c * 3 + 2]])
            else:
                rgb += bytes([c, c, c])
    png(sys.argv[2], w, h, rgb)


main()

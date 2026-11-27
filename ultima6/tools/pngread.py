#!/usr/bin/env python3
"""A small PNG reader (no PIL here): returns (w, h, rows of RGB tuples) for 8-bit RGB / RGBA / palette images; used to compare screenshots of the original (DOSBox) with ours."""
import struct
import sys
import zlib


def read_png(path):
    d = open(path, 'rb').read()
    assert d[:8] == b'\x89PNG\r\n\x1a\n'
    pos = 8
    idat = b''
    plte = None
    while pos < len(d):
        n, t = struct.unpack_from('>I4s', d, pos)
        body = d[pos + 8:pos + 8 + n]
        if t == b'IHDR':
            w, h, depth, ctype = struct.unpack('>IIBB', body[:10])
        elif t == b'PLTE':
            plte = body
        elif t == b'IDAT':
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    bpp = {2: 3, 6: 4, 3: 1, 0: 1}[ctype]
    stride = w * bpp
    rows = []
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = raw[p]
        cur = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for i in range(stride):
            a = cur[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                cur[i] = (cur[i] + a) & 255
            elif f == 2:
                cur[i] = (cur[i] + b) & 255
            elif f == 3:
                cur[i] = (cur[i] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                cur[i] = (cur[i] + pr) & 255
        prev = cur
        if ctype == 3:
            rows.append([tuple(plte[3 * v:3 * v + 3]) for v in cur])
        elif ctype == 0:
            rows.append([(v, v, v) for v in cur])
        else:
            rows.append([tuple(cur[x * bpp:x * bpp + 3]) for x in range(w)])
    return w, h, rows


if __name__ == '__main__':
    w, h, rows = read_png(sys.argv[1])
    print(w, h)

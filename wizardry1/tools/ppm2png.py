#!/usr/bin/env python3
"""ppm2png.py in.ppm out.png [scale]   -- Netpbm P6 -> PNG (scale = integer pixel multiplier)"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from conunit import write_png

def read_ppm(path):
    d = open(path, 'rb').read()
    pos, toks = 0, []
    while len(toks) < 4:
        while d[pos] in b' \n\r\t': pos += 1
        e = pos
        while d[e] not in b' \n\r\t': e += 1
        toks.append(d[pos:e]); pos = e
    pos += 1
    return int(toks[1]), int(toks[2]), d[pos:]

if __name__ == '__main__':
    w, h, px = read_ppm(sys.argv[1])
    sc = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    rows = []
    for y in range(h):
        line = bytearray()
        for x in range(w):
            line += px[(y * w + x) * 3:(y * w + x) * 3 + 3] * sc
        rows += [bytes(line)] * sc
    write_png(sys.argv[2], w * sc, h * sc, rows)

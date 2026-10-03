#!/usr/bin/env python3
"""ASCII view of one MAZEnn.DAT page (16x16; see docs/data-files.md).  usage: mm3_map.py MAZEnn.DAT
Each cell word holds four wall nibbles (side order N, E, S, W from the top nibble; N faces the higher row index = +y).
Wall type 0 = open; the low 3 bits give the wall style, bit 3 is a flag.  Rows are printed with y increasing upwards."""
import struct
import sys

d = open(sys.argv[1], "rb").read()
cells = [[struct.unpack_from("<H", d, (y * 16 + x) * 2)[0] for x in range(16)] for y in range(16)]
flags = [[d[0x200 + y * 16 + x] for x in range(16)] for y in range(16)]


def nib(w, s):
    return (w >> (12 - 4 * s)) & 0xF


out = []
for y in range(15, -1, -1):
    top = "+"
    mid = ""
    for x in range(16):
        w = cells[y][x]
        top += ("---" if nib(w, 0) & 7 else "   ") + "+"
        mid += ("|" if nib(w, 3) & 7 else " ") + (" . " if not flags[y][x] else "%02X " % (flags[y][x]))[:3]
    mid += "|" if nib(cells[y][15], 1) & 7 else " "
    out.append(top)
    out.append(mid)
out.append("+" + "".join(("---" if nib(cells[0][x], 2) & 7 else "   ") + "+" for x in range(16)))
print("\n".join(out))

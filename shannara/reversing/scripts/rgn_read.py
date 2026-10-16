"""
Reader for Shannara .RGN picture-region (hotspot) files, from the retail region loader (sub_7D8A4) and verified on all 12 files:
every region list ends exactly where the next one starts.

  directory: 6-byte entries {u16 dataOffset (0 = none), u16 baseId, u16 count}, one per picture entry of the same-numbered .PIC
             file; the directory ends at the smallest non-zero dataOffset.
  at dataOffset: `count` regions {i16 id, i16 type, ...}: type 2 = rectangle {x1, y1, x2, y2}; any other value = polygon with `type`
             vertices {x, y}. (The loader sorts rectangle corners and adds the picture window offset; in the 320x200 mode it
             scales x by 1/2 and y by 5/12.)

    python rgn_read.py SHAN020.RGN [entry]
"""
import struct
import sys


def read(path):
    d = open(path, "rb").read()
    ents = []
    m = len(d)
    i = 0
    while i * 6 < m:
        off, base, cnt = struct.unpack_from("<HHH", d, i * 6)
        ents.append([off, base, cnt, []])
        if off:
            m = min(m, off)
        i += 1
    for e in ents:
        off, base, cnt, regs = e
        p = off
        if not off:
            continue
        for _ in range(cnt):
            rid, typ = struct.unpack_from("<hh", d, p)
            p += 4
            if typ == 2:
                regs.append((rid, "rect", struct.unpack_from("<4h", d, p)))
                p += 8
            else:
                regs.append((rid, "poly", struct.unpack_from("<%dh" % (2 * typ), d, p)))
                p += 4 * typ
    return ents


if __name__ == "__main__":
    ents = read(sys.argv[1])
    print(len(ents), "entries,", sum(1 for e in ents if e[0]), "with regions")
    for i, e in enumerate(ents):
        if e[0] and (len(sys.argv) < 3 or int(sys.argv[2]) == i):
            print(i, "base", e[1], "count", e[2], e[3][:4])

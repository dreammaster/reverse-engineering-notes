#!/usr/bin/env python3
"""
Decompresses Ultima VI LZW files (u32 decompressed size + 9-12 bit LZW codes, see
src/files/lzw.h) for analysis: the *.DRV driver blobs are stored this way.

    lzw_extract.py <in> <out>
"""
import struct
import sys


def lzw(data, start=0):
    size = struct.unpack_from("<I", data, start)[0]
    pos = (start + 4) * 8
    out = bytearray()
    width, nxt, limit = 9, 0x102, 0x200
    dic = {}
    prev = last = None

    def read():
        nonlocal pos
        v = 0
        for i in range(width):
            v |= ((data[(pos + i) >> 3] >> ((pos + i) & 7)) & 1) << i
        pos += width
        return v

    while len(out) < size:
        c = read()
        if c == 0x101:
            break
        if c == 0x100:
            width, nxt, limit, dic = 9, 0x102, 0x200, {}
            prev = read()
            out.append(prev)
            last = prev
            continue
        cur = c
        stack = []
        if c >= nxt:
            stack.append(last)
            c = prev
        while c > 0xFF:
            p, ch = dic[c]
            stack.append(ch)
            c = p
        stack.append(c)
        out.extend(reversed(stack))
        dic[nxt] = (prev, c)
        nxt += 1
        prev, last = cur, c
        if nxt >= limit and width != 12:
            width += 1
            limit <<= 1
    return bytes(out)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    data = open(sys.argv[1], "rb").read()
    out = lzw(data)
    open(sys.argv[2], "wb").write(out)
    print("%s: %d -> %d bytes" % (sys.argv[1], len(data), len(out)))

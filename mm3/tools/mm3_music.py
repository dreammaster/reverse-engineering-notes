#!/usr/bin/env python3
"""Walk/dump an MM3 AdLib song (*.M).  Command = high nibble, channel = low nibble (derived from ADLIB.DRV, see docs/music.md).
usage: mm3_music.py SONG.M"""
import sys

ARGS = {0x0: 2, 0x1: 1, 0x2: 14, 0x3: 1, 0x4: 0, 0x5: 0, 0x6: 1, 0x7: 0, 0x8: 0, 0x9: 1, 0xA: 1, 0xB: 1, 0xC: 1, 0xD: 0, 0xE: 0, 0xF: 0}
NAMES = {0x0: "call", 0x1: "wait", 0x2: "instrument", 0x3: "skip1", 0x4: "nop", 0x5: "nop", 0x6: "skip1", 0x7: "noteOff", 0x8: "keyOff?",
         0x9: "note", 0xA: "volume", 0xB: "skip1", 0xC: "play", 0xD: "nop", 0xE: "nop", 0xF: "end"}


def walk(d, verbose=False):
    i, n = 0, 0
    while i < len(d):
        b = d[i]
        c, p = b >> 4, b & 15
        size = ARGS[c]
        if i + 1 + size > len(d):
            return -1, n
        if verbose:
            print("%04X %-10s ch%-2d %s" % (i, NAMES[c], p, d[i + 1:i + 1 + size].hex(" ")))
        i += 1 + size
        n += 1
    return i, n


if __name__ == "__main__":
    data = open(sys.argv[1], "rb").read()
    end, n = walk(data, True)
    print("; parsed %d commands, ended at %d of %d" % (n, end, len(data)))

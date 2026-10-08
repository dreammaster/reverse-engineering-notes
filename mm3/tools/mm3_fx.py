#!/usr/bin/env python3
"""Dump the sound-effect streams embedded in ADLIB.DRV (docs/music.md).

usage: mm3_fx.py [MM3.CC]          list every effect id with its stream offset and the decoded commands
       mm3_fx.py [MM3.CC] ID...    only the given effect ids

The driver holds a word table at offset 967h (one stream offset per effect id, ids 0-150); each stream is a command list whose
first byte is command (high nibble) + channel (low nibble).  Argument sizes come from the driver's effects dispatch table (1E5h):
 0 call(2)  1 wait(1)  2 instrument(11)  3 volume(1)  4,5,A,B nop  6 note-set(1)  7 key-off  8 note-set(1)  9 note-on(1)
 C select instrument(1)  D clear slide  E start slide(3)  F return (FFh) / other codes (FCh, FEh ...) stop the effect.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_cc  # noqa: E402

ARGS = {0x0: 2, 0x1: 1, 0x2: 11, 0x3: 1, 0x4: 0, 0x5: 0, 0x6: 1, 0x7: 0, 0x8: 1, 0x9: 1, 0xA: 0, 0xB: 0, 0xC: 1, 0xD: 0, 0xE: 3, 0xF: 0}
NAMES = {0x0: "call", 0x1: "wait", 0x2: "instrument", 0x3: "volume", 0x4: "nop", 0x5: "nop", 0x6: "pitch", 0x7: "keyOff", 0x8: "pitch",
         0x9: "note", 0xA: "nop", 0xB: "nop", 0xC: "useInstr", 0xD: "slideOff", 0xE: "slide", 0xF: "end"}
TABLE = 0x967
IDS = 151
SILENT = 0xB22


def load_driver(cc_path):
    d = open(cc_path, "rb").read()
    toc = {i: (o, s) for i, o, s in mm3_cc.read_toc(d)}
    m, _ = mm3_cc.member(d, *toc[mm3_cc.name_id("ADLIB.DRV")])
    return m


def walk(m, start, limit=400):
    i = start
    out = []
    for _ in range(limit):
        if i >= len(m):
            break
        b = m[i]
        c, ch = b >> 4, b & 15
        if c == 0xF:
            out.append("%04X end(%s)" % (i, "return" if b == 0xFF else "stop code %02X" % b))
            break
        n = ARGS[c]
        out.append("%04X %-9s ch%-2d %s" % (i, NAMES[c], ch, m[i + 1:i + 1 + n].hex(" ")))
        i += 1 + n
    return out


def main():
    args = sys.argv[1:]
    cc = args.pop(0) if args and args[0].upper().endswith(".CC") else r"D:\GOG Games\Might and Magic 3\MM3.CC"
    ids = [int(a) for a in args] or range(IDS)
    m = load_driver(cc)
    for fid in ids:
        off = struct.unpack_from("<H", m, TABLE + 2 * fid)[0]
        print("effect %3d -> stream %04X%s" % (fid, off, "  (silent)" if off == SILENT else ""))
        if off != SILENT:
            for line in walk(m, off):
                print("    " + line)


if __name__ == "__main__":
    main()

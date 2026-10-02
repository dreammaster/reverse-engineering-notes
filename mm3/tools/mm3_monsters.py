#!/usr/bin/env python3
"""Dump the 90 monsters of MM3 from the per-field Mon*.DAT members of MM3.CC.

usage: mm3_monsters.py DIR_WITH_EXTRACTED_CC_MEMBERS      (see mm3_cc.py extract)
Field layout (see docs/data-files.md): column arrays indexed by monster id (0-89).
"""
import os
import struct
import sys

FIELDS = [  # file, struct format
    ("MONHP", "<H"), ("MONAC", "B"), ("MONSPD", "B"), ("MONDMGN", "B"), ("MONDMGS", "B"), ("MONNUMA", "B"),
    ("MONEXP", "<I"), ("MONDMGT", "B"), ("MONATTP", "B"), ("MONSPEC", "B"), ("MONRANG", "B"), ("MONHITB", "B"),
    ("MONTREA", "B"), ("MONGOLD", "<I"), ("MONGEMS", "<H"), ("MONMAGI", "B"), ("MONFIRE", "B"), ("MONELEC", "B"),
    ("MONCOLD", "B"), ("MONACID", "B"), ("MONENER", "B"), ("MONPHYS", "B"),
]


def load(d):
    cols = {}
    for name, fmt in FIELDS:
        raw = open(os.path.join(d, name + ".DAT"), "rb").read()
        size = struct.calcsize(fmt)
        cols[name] = [struct.unpack_from(fmt, raw, i * size)[0] for i in range(len(raw) // size)]
    return cols


def main():
    cols = load(sys.argv[1])
    n = len(cols["MONHP"])
    print("id " + " ".join("%7s" % k[3:] for k, _ in FIELDS))
    for i in range(n):
        print("%2d " % i + " ".join("%7d" % cols[k][i] for k, _ in FIELDS))


if __name__ == "__main__":
    main()

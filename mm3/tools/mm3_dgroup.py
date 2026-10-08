#!/usr/bin/env python3
"""Dump the initialised DGROUP (data segment) of MM3.EXE as a flat 64 KB file, so that the C code can read the game's lookup
tables at run time by DGROUP offset (the offsets used all over docs/ and names/mm3.tsv) without embedding game data in the source.

usage: mm3_dgroup.py MM3.EXE DGROUP.BIN
DGROUP starts at IDA linear 286F0h = load-image offset 186F0h (selector 286Fh, see docs/exe-layout.md)."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import unpack_mm3  # noqa: E402

DGROUP_LINEAR = 0x286F0
IMAGE_BASE = 0x10000


def main():
    f = open(sys.argv[1], "rb").read()
    hdr, img, _, _, _, _ = unpack_mm3.unpack_outer(f)
    image, _, _, _ = unpack_mm3.unpack_exepack(hdr + img)
    start = DGROUP_LINEAR - IMAGE_BASE
    seg = image[start:start + 0x10000]
    seg += bytes(0x10000 - len(seg))
    open(sys.argv[2], "wb").write(seg)
    print("wrote %d bytes of DGROUP (image offset %Xh)" % (len(seg), start))


if __name__ == "__main__":
    main()

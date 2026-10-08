#!/usr/bin/env python3
"""Dump the program image the way the game sees it in memory: the unpacked, relocated root image (load segment 1000h) followed by
all 13 Borland overlays at the addresses IDA uses, as one flat block for linear addresses 10000h..52165h.  The translated C code
(src/gen) and the host layer address it as machine memory (DGROUP is at 286F0h inside it).

usage: mm3_image.py MM3.EXE IMAGE.BIN"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm3_emu  # noqa: E402

IMAGE_START, IMAGE_END = 0x10000, 0x52165


def main():
    e = mm3_emu.Emu(sys.argv[1])
    img = bytes(e.uc.mem_read(IMAGE_START, IMAGE_END - IMAGE_START))
    open(sys.argv[2], "wb").write(img)
    print("wrote %d bytes (linear %Xh-%Xh)" % (len(img), IMAGE_START, IMAGE_END))


if __name__ == "__main__":
    main()

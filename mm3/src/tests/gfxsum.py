#!/usr/bin/env python3
"""Prints what src/gfxsum prints, from the Python reference decoder.  usage: gfxsum.py MM3.CC"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
import mm3_cc  # noqa: E402
import mm3_gfx  # noqa: E402


def fnv(h, b):
    for x in b:
        h = ((h ^ x) * 16777619) & 0xFFFFFFFF
    return h


d = open(sys.argv[1], "rb").read()
for ident, off, size in mm3_cc.read_toc(d):
    m, _ = mm3_cc.member(d, off, size)
    if len(m) == 64000:
        print("%04X raw %08x" % (ident, fnv(2166136261, m)))
        continue
    try:
        frames = mm3_gfx.decode_sprite(m)
    except Exception:  # noqa: BLE001
        continue
    if not frames:
        continue
    h = 2166136261
    for w, hh, px in frames:
        h = fnv(h, bytes((w & 255, w >> 8, hh & 255, hh >> 8)))
        h = fnv(h, px)
    print("%04X sprite %d %08x" % (ident, len(frames), h))

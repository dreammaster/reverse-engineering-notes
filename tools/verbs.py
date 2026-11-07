#!/usr/bin/env python3
"""
Verb icons shown in the six panel slots while the player touches an object (CAD.EXE 1000:22C4 builds the list, 1000:899E / 8A4A draw it).
docs/formats.md 6.7b.  Icons = ANDATA block 0: 17 icons of 16 x 14 pixels, 8 bytes per row (4 big-endian plane words), colour 0 transparent.
"""
import struct

SLOTS = ((9, 145), (36, 152), (9, 169), (36, 177), (63, 175), (90, 177))     # ds:FD8A (x, y) of the six slots
CLASS_VERB = (0x00, 0x04, 0x09, 0xFF, 0x07, 0x03, 0x08, 0xFF, 0xFF, 0xFF)    # ds:FA36 + class (sprite header byte 0x17)


def verb_list(hdr, flags2, ds703=0):
    """hdr = 32-byte sprite block header (file order), flags2 = rec[15] of the touched thing. -> list of verb numbers (icons), at most 6."""
    v = []
    h12, h16, h17, h1d = hdr[12], hdr[0x16], hdr[0x17], hdr[0x1D]
    if h12 & 2 and not flags2 & 4:
        v.append(2)
    if h17 and not flags2 & 0x40:
        c = CLASS_VERB[h17] if h17 < len(CLASS_VERB) else 0xFF
        if not c & 0x80:
            v.append(c)
    if h1d == 0x12:
        v.append(0x0E)
    if h12 & 1 and not flags2 & 4 and not ds703 & 0x20:
        v.append(0x0A)
    if h17 != 6 and not h16 & 0x80:
        v.append(0x0B)
    v.append(6)
    return v


def draw_icon(canvas, x, y, icons, n):
    for r in range(14):
        ws = struct.unpack('>4H', icons[n * 112 + r * 8:n * 112 + r * 8 + 8])
        for b in range(16):
            c = sum(((ws[p] >> (15 - b)) & 1) << p for p in range(4))
            if c and 0 <= x + b < 320 and 0 <= y + r < 200:
                canvas[(y + r) * 320 + x + b] = c


def draw_verbs(canvas, verbs, icons):
    """All six slots: the verbs' icons, then the empty icon 0."""
    for i in range(6):
        draw_icon(canvas, SLOTS[i][0], SLOTS[i][1], icons, verbs[i] if i < len(verbs) else 0)

#!/usr/bin/env python3
"""
The screen's lower frame, drawn from ANDATA.DAT block 1 by CAD.EXE 1000:D523 (docs/formats.md 6.3).

Rows 115..199 get two wedge-shaped wings (the diamond-shaped room view is cut by diagonal edges; everything outside is
frame).  Block 1 holds, per row, ST-interleaved 16-pixel groups (4 big-endian plane words each):
    (cl+1) left wing groups, 1 left edge group, 1 right edge group, (cl+1) right wing groups
where cl = -1 for the first 8 rows, then 0 for the next 8, ... (one more group every 8 rows).  The two edge groups are
ORed over pixels that a stair-step mask (2 bits shift per row) lets through.
"""
import struct


def draw_wings(b1, planes=None):
    """-> planes: 4 bytearrays of 8000 bytes (320x200, 40 bytes per row, plane p = colour bit p)."""
    if planes is None:
        planes = [bytearray(8000) for _ in range(4)]
    si = 0
    row = 115
    cl = -1
    bp = bx = 0xFFFF

    def put_word(p, off, w):
        planes[p][off] = w >> 8
        planes[p][off + 1] = w & 255

    def get_word(p, off):
        return (planes[p][off] << 8) | planes[p][off + 1]

    def group(si):
        return struct.unpack('>4H', b1[si:si + 8])

    while row < 200:
        di = row * 40
        if cl >= 0:
            for g in range(cl + 1):
                ws = group(si)
                for p in range(4):
                    put_word(p, di + 2 * g, ws[p])
                si += 8
            di += 2 * (cl + 1)
        # left edge: AND all planes with bx, OR the group
        for p in range(4):
            put_word(p, di, get_word(p, di) & bx)
        ws = group(si)
        for p in range(4):
            put_word(p, di, get_word(p, di) | ws[p])
        si += 8
        di += 2
        di += 36 - 4 * (cl + 1) if cl >= 0 else 36
        # right edge
        for p in range(4):
            put_word(p, di, get_word(p, di) & bp)
        ws = group(si)
        for p in range(4):
            put_word(p, di, get_word(p, di) | ws[p])
        si += 8
        di += 2
        if cl >= 0:
            for g in range(cl + 1):
                ws = group(si)
                for p in range(4):
                    put_word(p, di + 2 * g, ws[p])
                si += 8
        row += 1
        bp = (bp << 2) & 0xFFFF
        bx >>= 2
        if bx == 0:
            cl += 1
            bp = bx = 0xFFFF
    return planes, si


def apply_wings(canvas, b1):
    """Chunky version: draw the wings over a 320x200 canvas of colour indices (0xFF = empty is treated as 0), in place.
    The stair-step masks keep the old pixels of the two edge groups where the mask bit is set."""
    for i, c in enumerate(canvas):
        if c == 0xFF:
            canvas[i] = 0

    def put(row, g, ws, keep=None):
        for b in range(16):
            col = sum(((ws[p] >> (15 - b)) & 1) << p for p in range(4))
            o = row * 320 + g * 16 + b
            if keep is None:
                canvas[o] = col
            else:
                canvas[o] = (canvas[o] if (keep >> (15 - b)) & 1 else 0) | col

    si, row, cl, bp, bx = 0, 115, -1, 0xFFFF, 0xFFFF
    grp = lambda s: struct.unpack('>4H', b1[s:s + 8])
    while row < 200:
        for g in range(cl + 1):
            put(row, g, grp(si)); si += 8
        put(row, cl + 1, grp(si), bx); si += 8
        put(row, 18 - cl, grp(si), bp); si += 8
        for g in range(cl + 1):
            put(row, 19 - cl + g, grp(si)); si += 8
        row += 1
        bp = (bp << 2) & 0xFFFF
        bx >>= 2
        if bx == 0:
            cl += 1
            bp = bx = 0xFFFF
    return canvas

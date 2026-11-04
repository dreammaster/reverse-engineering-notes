#!/usr/bin/env python3
"""
Room exits (door boxes) = the entries the game builds at ss:[3A] in CAD.EXE 1000:4D00..4ECD, docs/formats.md 6.12.

The room record (MP2 section 0) lists door ids at +6 (u16be words, FFFF = unused, `rec[0x16]` of the expanded record = how many);
a door id indexes MP2 section 1 (8-byte records): (x, y) = global map cell, +4 = length in cells (1 or 2), +5 = orientation (0 = the
door lies along y, 1 = along x).  The room record gives the room's own map origin (+1 = x, +3 = y).
"""
import struct


def s8(v):
    v &= 0xFF
    return v - 256 if v & 0x80 else v


def room_exits(room_rec, doors, headers, thick_x, thick_y):
    """room_rec: compact room record; doors: MP2 section 1 records; headers: callable(block) -> (xs, ys, zs).
    -> [(x1, y1, x0, y0, zt, zb, door id)] in record order."""
    out = []
    if len(room_rec) < 22:
        return out
    words = [struct.unpack('>H', room_rec[6 + 2 * i:8 + 2 * i])[0] for i in range(7)]
    words = [w for w in words if w != 0xFFFF]
    for d in words:
        if d >= len(doors) or len(doors[d]) < 6:
            continue
        r = doors[d]
        dl, dh, ln, ori = r[0], r[1], r[4], r[5]
        if ori == 0:
            ch = 2
            dl = s8(dl - room_rec[1])
            if dl < 0:
                dl += 1
                ch = 0
            dh = s8(dh - room_rec[3])
        else:
            ch = 3
            dh = s8(dh - room_rec[3])
            if dh < 0:
                dh += 1
                ch = 1
            dl = s8(dl - room_rec[1])
        hd = headers((ln - 1) * 4 + ch) if ln >= 1 else None
        if hd is None:
            continue
        xs, ys, zs = hd
        x0, y0 = (dl * 8) & 0xFF, (dh * 8) & 0xFF
        dl, dh = x0, y0
        zb = 0
        bx, by = xs, ys
        if xs == 8:
            bx = thick_x
            y0 = (y0 - 4) & 0xFF
            dh = (dh + 4) & 0xFF
        if ys == 8:
            by = thick_y
            x0 = (x0 - 4) & 0xFF
            dl = (dl + 4) & 0xFF
        x1 = (dl + bx - 1) & 0xFF
        y1 = (dh + by - 1) & 0xFF
        out.append((x1, y1, x0, y0, (zb + zs - 1) & 0xFF, zb, d))
    return out

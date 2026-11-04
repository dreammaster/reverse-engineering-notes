#!/usr/bin/env python3
"""
Reference model of CAD.EXE sub_116E0 (collision_test) and sub_11968 (exit lookup), docs/formats.md 6.12.
It works on a snapshot of the game's data segment (`ds`, 64 KB) so it can be compared with the original routine running in the
emulator (tools/verify_collision.py).  Inputs as in the original: dl, dh = x1, y1; ch = z of the mover; (xs, ys, zs) = its box
sizes (sprite header words +10h, +12h, +14h); si = the mover's position record pointer (0xFFFF = none).
Returns dict(zf, count, flag, ptrs, di): count/flag = the two bytes at the start of the result list (flag FDh = solid hit,
count FFh = blocked by the room, FEh = exit found), ptrs = position record pointers of the overlapped objects.
Entry layout (50 bytes): x1 y1 x0 y0 z_top z_base (signed bytes) ... +0Ah position record, +14h solid marker.
"""


def s8(v):
    v &= 0xFF
    return v - 256 if v & 0x80 else v


def w(ds, o):
    return ds[o] | (ds[o + 1] << 8)


def overlap(hi, lo, e1, e0):
    """The game's inclusive interval test on signed bytes: [lo, hi] against [e0, e1]."""
    hi, lo, e1, e0 = s8(hi), s8(lo), s8(e1), s8(e0)
    if hi >= e1:
        return lo <= e1
    return hi >= e0


def exit_lookup(ds, si, dl, dh, cl, bl, bh, ch, ss=None):
    """sub_11968 -> (al, di).  The exit entries are addressed through bp, i.e. in the STACK segment (ss); ss defaults to ds."""
    ss = ds if ss is None else ss
    if si == 0xFFFF:
        return 0xFF, 0
    if w(ds, si + 4) != 0 and ds[(si + ds[si + 0x0C] + 5) & 0xFFFF] != 7:
        return 0xFF, 0
    cnt = w(ds, 0x22E)
    if cnt == 0:
        return 0xFF, 0
    bp = w(ds, 0x3A)
    ah = cnt & 0xFF
    while True:
        if w(ss, bp) != 0xFFFF and overlap(dl, cl, ss[bp], ss[bp + 2]) and overlap(dh, bl, ss[bp + 1], ss[bp + 3]) \
                and overlap(bh, ch, ss[bp + 4], ss[bp + 5]):
            hit = True
            if w(ds, si + 4) == 0:
                u = lambda v: v & 0xFF
                zok = s8(ch) >= s8(ss[bp + 5]) and s8(bh) <= s8(ss[bp + 4])
                if ds[0x5F9] & 3:
                    hit = u(cl) >= u(ss[bp + 2]) and u(dl) < u(ss[bp + 0]) and zok
                else:
                    hit = u(bl) >= u(ss[bp + 3]) and u(dh) < u(ss[bp + 1]) and zok
            if hit:
                return 0xFE, w(ss, bp + 0x0A)
        bp += 0x32
        ah -= 1
        if ah < 0:
            return 0xFF, 0


def collision_test(ds, dl, dh, ch, xs, ys, zs, si, ss=None):
    cl = (dl - xs + 1) & 0xFF
    bl = (dh - ys + 1) & 0xFF
    bh = (ch + zs - 1) & 0xFF
    count = ds[0x224]
    ptrs, flag = [], 0
    res = dict(zf=1, count=0, flag=0, ptrs=ptrs, di=None)
    out = (s8(ch) < 0 or bh >= 0xF0 or s8(dl) >= s8(ds[0x5F4]) or s8(dh) >= s8(ds[0x5F5]) or
           (not s8(bl) > s8(ds[0x5E0]) and s8(cl) <= s8(ds[0x5DF])) or s8(cl) < s8(ds[0x5D2]) or not s8(bl) >= s8(ds[0x5D1]))
    if out:
        al, di = exit_lookup(ds, si, dl, dh, cl, bl, bh, ch, ss)
        res.update(zf=0, count=al, di=di)
        return res
    zones = w(ds, 0x54)
    if zones and not (s8(cl) >= 8 and s8(bl) >= 8):
        base = {0x1782: 0x5F0, 0x17B1: 0x5EC, 0x17DF: 0x5E8, 0x180D: 0x5E4}.get(zones & 0xFFFF)
        if base is None:
            raise ValueError('unknown zone entry %#x' % zones)
        for z in range(base, 0x5E4 - 4, -4):
            if overlap(dl, cl, ds[z], ds[z + 2]) and overlap(dh, bl, ds[z + 1], ds[z + 3]):
                res.update(zf=0, count=0xFF)
                return res
    di = w(ds, 0x2A)
    for _ in range(count):
        if w(ds, di) & 0x8000:
            di += 0x32
            continue
        if si == w(ds, di + 0x0A):
            di += 0x32
            continue
        if overlap(dl, cl, ds[di], ds[di + 2]) and overlap(dh, bl, ds[di + 1], ds[di + 3]) and \
                overlap(bh, ch, ds[di + 4], ds[di + 5]):
            rec = w(ds, di + 0x0A)
            ptrs.append(rec)
            if s8(ds[di + 0x14]) < 0 or ds[(rec + 0x0F) & 0xFFFF] & 4:
                flag = 0xFD
        di += 0x32
    res.update(count=len(ptrs), flag=flag, zf=1 if not ptrs else 0)
    return res

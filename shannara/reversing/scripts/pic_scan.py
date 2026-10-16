"""
Scans Shannara .PIC files (retail picture containers, extracted from PICS.UHA) and checks the directory layout
documented in analysis/retail_game_layer.md section 12/22.

    python pic_scan.py SHAN020.PIC [-v] [-png outdir]
"""
import collections
import struct
import sys


def entries(d):
    out = []
    i = 0
    while i * 12 + 12 <= len(d):
        off, = struct.unpack_from("<I", d, i * 12)
        if off == 0:
            break
        count, flags, w, h, b10, b11 = struct.unpack_from("<BBHHBB", d, i * 12 + 4)
        out.append(dict(i=i, off=off, count=count, flags=flags, w=w, h=h, b10=b10, b11=b11))
        i += 1
        if i * 12 >= out[0]["off"]:
            break
    for k, e in enumerate(out):
        e["end"] = out[k + 1]["off"] if k + 1 < len(out) else len(d)
        e["span"] = e["end"] - e["off"]
    return out


def load_entry(d, e):
    """returns (palette or None, offset (x, y), pixels bytes); pixel data is w*h bytes, 8 bpp"""
    base = e["off"]
    xy = (0, 0)
    if e["count"]:
        xy = struct.unpack_from("<hh", d, base)
        base += 4 * e["count"]
    pal = None
    if e["flags"] & 0x10:
        pal = d[base:base + 768]
        base += 768
    return pal, xy, d[base:base + e["w"] * e["h"]]


def composite_xor(bg, bg_w, spr, sx, sy, sw, sh):
    """flag 0x40 sprites: index 0 = unchanged, otherwise destination ^= sprite pixel (verified visually on SHAN020 entries 3 + 9)"""
    out = bytearray(bg)
    for y in range(sh):
        for x in range(sw):
            out[(sy + y) * bg_w + sx + x] ^= spr[y * sw + x]
    return out


if __name__ == "__main__":
    d = open(sys.argv[1], "rb").read()
    es = entries(d)
    flags = collections.Counter()
    ok = bad = 0
    for e in es:
        pal = 0x300 if e["flags"] & 0x10 else 0
        e["payload"] = e["span"] - e["count"] * 4 - pal
        exp1 = e["w"] * e["h"]
        flags[(e["flags"], e["count"] > 0)] += 1
        match = e["payload"] == exp1 * max(1, e["count"])
        ok += match
        bad += not match
        if "-v" in sys.argv:
            print("%3d off=%8d count=%2d flags=%02x w=%4d h=%4d b10=%02x b11=%02x span=%8d payload=%8d w*h=%7d %s" % (e["i"], e["off"], e["count"], e["flags"], e["w"], e["h"], e["b10"], e["b11"], e["span"], e["payload"], exp1, "" if match else "<<"))
    print(sys.argv[1], "entries:", len(es), "payload == w*h*max(1,count):", ok, "other:", bad, "flag values:", dict(flags))

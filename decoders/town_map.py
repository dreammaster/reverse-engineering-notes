#!/usr/bin/env python3
"""Decode the Legacy of the Ancients interior tile maps that `TWNDR.EXE`
and `CASDR.EXE` share through the `bmTNCALB` renderer:

    TOWN0.BSV .. TOWNB.BSV   the 12 towns          (80 x 40)
    CASTLE.BS1 / .BS2        the two castle floors (90 x 91)
    FORT.BS1 / .BS2          the two fort floors   (112 x 73)

    python decoders/town_map.py [TOWN0|CASTLE.BS1|FORT.BS1] [C:\\games\\lota]

Each file is a Microsoft BASIC `BSAVE` image. `loadTownData` /
`loadCastleLevel` `BLOAD` it into the shared map array (`ds:1E2A`);
`setViewport` picks the stride/height:

    mode 0  mapStride 0x50 (80)  mapHeight 0x28 (40)   towns   (size 0xC80)
    mode 1  mapStride 0x5A (90)  mapHeight 0x5B (91)   castles
    mode 2  mapStride 0x70 (112) mapHeight 0x49 (73)   forts

Town payload: map (`0x000`..) then object/feature records, a 192-byte
`0x1A` slot table at `0x1000`, padding, and the town's shop-name strings
as wide chars from `0x1300`. Castle/fort: map, a per-floor table (where
`.BS1` vs `.BS2` stop differing, ~`0x2000`), then a shared CGA 2 bpp
tile-graphic bank (`~0x2400`+). Tile bytes are graphic indices; the
code->graphic map is in `TWNDR`/`CASDR` + `TCASOBJ.BSV`.
"""
import os
import struct
import sys

LAYOUTS = {          # stem prefix -> (width, height, text_offset or None)
    "TOWN": (80, 40, 0x1300),
    "CASTLE": (90, 91, None),
    "FORT": (112, 73, None),
}

GLYPH = {0x00: "  ", 0xA9: "..", 0x3A: "~~"}


def read_bsave(path):
    b = open(path, "rb").read()
    if b[0] != 0xFD:
        raise ValueError(f"{path}: not a BSAVE image")
    seg, off, ln = struct.unpack_from("<HHH", b, 1)
    return b[7:7 + ln]


def layout_for(name):
    for pre, spec in LAYOUTS.items():
        if name.upper().startswith(pre):
            return spec
    raise ValueError(f"unknown map file: {name}")


def town_names(payload, text_off):
    if text_off is None:
        return []
    t = payload[text_off:]
    out, cur = [], []
    for i in range(0, len(t) - 1, 2):
        lo, hi = t[i], t[i + 1]
        if hi == 0 and 0x20 <= lo < 0x7F:
            cur.append(chr(lo))
        else:
            if len(cur) > 2:
                out.append("".join(cur))
            cur = []
    if len(cur) > 2:
        out.append("".join(cur))
    return out


def dump(name, games_dir):
    fname = name if "." in name else name + ".BSV"
    p = read_bsave(os.path.join(games_dir, fname))
    w, h, text_off = layout_for(name)
    print(f"{fname}  payload {len(p)} bytes   map {w} x {h}")
    m = p[:w * h]
    for r in range(h):
        row = m[r * w:(r + 1) * w]
        print("".join(GLYPH.get(b, f"{b:02X}") for b in row))
    names = town_names(p, text_off)
    if names:
        print("\nshop / service names:")
        for s in names:
            print(f"  {s}")


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "TOWN0"
    games = sys.argv[2] if len(sys.argv) > 2 else r"C:\games\lota"
    dump(name, games)


if __name__ == "__main__":
    main()

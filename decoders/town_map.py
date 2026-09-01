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

Interiors render exactly like the overworld: `drawInteriorTiles`
(`bmTNCALB`) blits a **26 x 17 grid of 8x8 cells** with `rtm_FE1B`,
sourced from the map array at `ds:2082`.

Town payload: map (`0x000`..`w*h`) then at `~0xC80` a tile-code list +
**4-byte object records** `(x, y, type, 0)` for the shops/doors +
multi-cell building shapes; a 192-byte `0x1A` slot table at `0x1000`;
a small word param table at `~0x12C0`; the shop-name strings as
length-prefixed wide chars from `0x1300`.  Towns carry **no tile-graphic
bank** -- they use the shared `bmTNCALB` tile set.

Castle/fort: map, a per-floor table (where `.BS1` vs `.BS2` stop
differing, `~0x2000`), then a **~234-cell CGA 2 bpp tile-graphic bank at
`0x2400`** (field-interleaved 8x8 cells, low-byte-first -- walls,
floors, doorframes, windows, torches, decoration).  Tile bytes are
graphic indices into it.  `TCASOBJ.BSV` supplies animated-object frames
(banners / torches / gates) overlaid on specific tiles;
`FORTANIM.BSV` swaps `TCASOBJ`'s last `0x100` block for the fort.

Pass `--bank` to render the castle/fort `0x2400` graphic bank to a PNG.
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


PAL = [(0, 0, 0), (0x55, 0xFF, 0xFF), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def _cell(p, o):
    w = struct.unpack("<8H", p[o:o + 16].ljust(16, b"\0"))
    rows = []
    for wd in (w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]):
        lo, hi = wd & 0xFF, wd >> 8
        rows.append([(lo >> 6) & 3, (lo >> 4) & 3, (lo >> 2) & 3, lo & 3,
                     (hi >> 6) & 3, (hi >> 4) & 3, (hi >> 2) & 3, hi & 3])
    return rows


def render_bank(name, games_dir, out, start=0x2400, cols=24, scale=4):
    import zlib
    fname = name if "." in name else name + ".BS1"
    p = read_bsave(os.path.join(games_dir, fname))
    n = (len(p) - start) // 16
    rn = (n + cols - 1) // cols
    grid = [[0] * (9 * cols) for _ in range(9 * rn)]
    for k in range(n):
        c = _cell(p, start + k * 16)
        ox, oy = (k % cols) * 9, (k // cols) * 9
        for y in range(8):
            for x in range(8):
                grid[oy + y][ox + x] = c[y][x]
    h, w = len(grid), len(grid[0])
    raw = bytearray()
    for row in grid:
        line = bytearray()
        for v in row:
            line += bytes(PAL[v]) * scale
        for _ in range(scale):
            raw.append(0)
            raw += line

    def ch(t, d):
        return (struct.pack(">I", len(d)) + t + d
                + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF))
    open(out, "wb").write(
        b"\x89PNG\r\n\x1a\n"
        + ch(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0))
        + ch(b"IDAT", zlib.compress(bytes(raw), 9)) + ch(b"IEND", b""))
    print(f"wrote {out}  ({n} cells from 0x{start:X})")


def main():
    argv = sys.argv[1:]
    bank_out = None
    if "--bank" in argv:
        i = argv.index("--bank")
        bank_out = argv[i + 1] if i + 1 < len(argv) else "castle_bank.png"
        argv = argv[:i] + argv[i + 2:]
    name = argv[0] if argv else "TOWN0"
    games = argv[1] if len(argv) > 1 else r"C:\games\lota"
    if bank_out:
        render_bank(name, games, bank_out)
        return
    dump(name, games)


if __name__ == "__main__":
    main()

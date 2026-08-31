#!/usr/bin/env python3
"""Decode `TOWN0.BSV` .. `TOWNB.BSV` -- the 12 Legacy of the Ancients town
layouts (used by `TWNDR.EXE`).

    python decoders/town_map.py [TOWN0] [C:\\games\\lota]

Each file is a Microsoft BASIC `BSAVE` image (7-byte header; seg 0x9259,
off 0). `TWNDR`'s `loadTownData` `BLOAD`s `TOWN<n>.BSV` into the shared
map array (`ds:1E2A`) and `setViewport` mode 0 reads it with
`mapStride = 0x50` (80) and `mapHeight = 0x28` (40) -- `ds:253Ah` etc.
hold the map size `0xC80` = 3200.

Payload layout (total ~5000-5120 B; only the text tail varies per town):

    0x000 - 0xC7F   town map: 80 wide x 40 tall, 1 byte per tile
    0xC80 - 0xFFF   object / feature records (multi-tile shop art, etc.)
    0x1000- 0x10BF  a 192-byte table of 0x1A (door / NPC slots -- TBD)
    0x10C0- 0x12FF  zero padding
    0x1300- end     the town's shop / service names as wide chars
                    (`XX 00` per char), separated by control bytes

Tile codes seen (in `TOWN0`): 0xA9 open ground, 0x0E-0x12 street/path,
0x48-0x4B building walls, 0x67-0x6F building interior, 0x3A water,
0x5B / 0x9C-0xD0 shop features, 0x70-0x89 / 0xB2-0xB6 decorations,
0x00 out-of-bounds. The code->tile-graphic mapping is in `TWNDR` +
`TCASOBJ.BSV`.
"""
import os
import struct
import sys

W, H = 80, 40
MAP_BYTES = W * H          # 0xC80
TEXT_OFF = 0x1300

GLYPH = {0x00: "  ", 0xA9: "..", 0x3A: "~~"}


def read_bsave(path):
    b = open(path, "rb").read()
    if b[0] != 0xFD:
        raise ValueError(f"{path}: not a BSAVE image")
    seg, off, ln = struct.unpack_from("<HHH", b, 1)
    return b[7:7 + ln]


def town_names(payload):
    t = payload[TEXT_OFF:]
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
    p = read_bsave(os.path.join(games_dir, name + ".BSV"))
    print(f"{name}.BSV  payload {len(p)} bytes")
    m = p[:MAP_BYTES]
    for r in range(H):
        row = m[r * W:(r + 1) * W]
        print("".join(GLYPH.get(b, f"{b:02X}") for b in row))
    print("\nshop / service names:")
    for s in town_names(p):
        print(f"  {s}")


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "TOWN0"
    games = sys.argv[2] if len(sys.argv) > 2 else r"C:\games\lota"
    dump(name, games)


if __name__ == "__main__":
    main()

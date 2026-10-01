"""
Top-down map renderer: python mm2_maps.py MAP OUT.png [scale]
Walls: N = bits 6-7, E = 4-5, S = 2-3, W = 0-1 of the wall byte (0 open, 1 wall, 2 door, 3 secret/sprite).
y grows to the north, so row 15 is drawn at the top.  Cells with the event-trigger flag (bit 7 of the flag
byte) get a red dot.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm2_gfx as g          # noqa: E402
from mm2_data import Game    # noqa: E402

PAL = [(0, 0, 0), (255, 255, 255), (0, 170, 255), (255, 170, 0), (255, 0, 0), (60, 60, 60)]


def render(m, scale=12):
    data = Game().map(m)
    walls, flags = data[:256], data[256:512]
    n = 16 * scale
    img = [[0] * n for _ in range(n)]
    for y in range(16):
        for x in range(16):
            w = walls[y * 16 + x]
            px, py = x * scale, (15 - y) * scale
            sides = {"N": (w >> 6) & 3, "E": (w >> 4) & 3, "S": (w >> 2) & 3, "W": w & 3}
            for s, v in sides.items():
                if not v:
                    continue
                col = {1: 1, 2: 2, 3: 3}[v]
                for k in range(scale):
                    if s == "N":
                        img[py][px + k] = col
                    elif s == "S":
                        img[py + scale - 1][px + k] = col
                    elif s == "W":
                        img[py + k][px] = col
                    else:
                        img[py + k][px + scale - 1] = col
            if flags[y * 16 + x] & 0x80:
                c = scale // 2
                img[py + c][px + c] = 4
    return n, img


if __name__ == "__main__":
    s = int(sys.argv[3]) if len(sys.argv) > 3 else 12
    n, img = render(int(sys.argv[1]), s)
    g.write_png(sys.argv[2], n, n, img, PAL)

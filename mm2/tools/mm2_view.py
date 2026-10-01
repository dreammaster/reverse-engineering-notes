"""
Software renderer for the MM2 indoor first-person view, written from 2PLAY draw_view_indoors,
wall_collect (sub_1BEBA) and view_draw_wall_a/b/c.  Used to verify docs/view.md.

    python mm2_view.py MAP X Y FACING OUT.png [town|cave|castle]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mm2_gfx as g          # noqa: E402
from mm2_data import Game    # noqa: E402

GAME = r"D:\GOG Games\Might and Magic 2" + "\\"

# DGROUP:153E.. tables (x, y per depth index 0..3)
A_X, A_Y = [32, 64, 88, 104], [22, 40, 54, 62]
BL_X, BL_Y = [8, 32, 64, 88], [8, 22, 40, 54]                     # left side wall
BO_IMG, BO_X, BO_Y = [12, 14, 2, 3], [8, 8, 40, 88], [22, 40, 54, 62]   # left lane front wall
CL_X, CL_Y = [192, 160, 136, 120], [8, 22, 40, 54]                # right side wall
CO_IMG, CO_X, CO_Y = [13, 15, 2, 3], [192, 160, 136, 120], [22, 40, 54, 62]

# facing -> (front mask, shift) ; wall byte: N = bits 6-7, E = 4-5, S = 2-3, W = 0-1
FACE = {"N": (0xC0, 6), "E": (0x30, 4), "S": (0x0C, 2), "W": (0x03, 0)}
# step, left-lane offset, right-lane offset (y grows to the north)
STEP = {"N": ((0, 1), (-1, 0), (1, 0)), "S": ((0, -1), (1, 0), (-1, 0)),
        "E": ((1, 0), (0, 1), (0, -1)), "W": ((-1, 0), (0, -1), (0, 1))}


def collect(walls, x, y, facing):
    """-> dict of the 20 wall variables, keyed (kind, depth 1-4); kind: L side, LO left-lane front,
    R side, RO right-lane front, F front.  Mirrors wall_collect."""
    fm, fs = FACE[facing]
    lm = {"N": 0x03, "E": 0xC0, "S": 0x30, "W": 0x0C}[facing]
    rm = {"N": 0x30, "E": 0x03, "S": 0xC0, "W": 0xC0 >> 0 if False else 0x0C}[facing]
    rm = {"N": 0x30, "E": 0x0C, "S": 0x03, "W": 0xC0}[facing]
    ls, rs = (fs + 2) & 7, ((fs - 2) if fs else 6)
    (sx, sy), (lx, ly), (rx, ry) = STEP[facing]

    def cell(cx, cy):
        return walls[((cy & 15) << 4) | (cx & 15)]
    ctr = [cell(x + sx * d, y + sy * d) for d in range(4)]
    lft = [cell(x + lx + sx * d, y + ly + sy * d) for d in range(4)]
    rgt = [cell(x + rx + sx * d, y + ry + sy * d) for d in range(4)]
    out = {}
    for d in range(4):
        n = d + 1
        v = (ctr[d] & lm) >> ls
        if v:
            out[("L", n)] = v
        else:
            o = (lft[d] & fm) >> fs
            if o:
                out[("LO", n)] = o
        v = (ctr[d] & rm) >> rs
        if v:
            out[("R", n)] = v
        else:
            o = (rgt[d] & fm) >> fs
            if o:
                out[("RO", n)] = o
        f = (ctr[d] & fm) >> fs
        if f:
            out[("F", n)] = f
            # open neighbours: the next lane cell's front wall shows past the opening
            if d < 3:
                if ("L", n) not in out and ("LO", n) not in out:
                    o = (lft[d + 1] & fm) >> fs
                    if o:
                        out[("LO", n + 1)] = o
                if ("R", n) not in out and ("RO", n) not in out:
                    o = (rgt[d + 1] & fm) >> fs
                    if o:
                        out[("RO", n + 1)] = o
            break
    for k, o in (("L", "LO"), ("R", "RO")):
        for n in (1, 2):
            if (k, n) in out and out.get((o, n + 1)) == 3:
                out[(o, n + 1)] = 1
    return out


def render(game_dir, map_id, x, y, facing, style="TOWN"):
    walls = Game(game_dir.rstrip("\\")).map(map_id)[:256]
    bank = lambda n: g.bank(open(GAME + n + ".16", "rb").read())   # noqa: E731
    wb, fl, sky = bank(style), bank(style + "F"), bank("SKY")
    canvas = [[0] * 320 for _ in range(200)]

    def blit(b, idx, px, py):
        io, mo = g.entries(b)[idx]
        w, h, pix = g.image(b, io, 4)
        m = g.mask(b, mo, w, h) if mo else None
        for j in range(h):
            for i in range(w):
                if (m is None or m[j][i]) and 0 <= py + j < 200 and 0 <= px + i < 320:
                    canvas[py + j][px + i] = pix[j][i]
    blit(sky, 1, 8, 8)
    blit(fl, 0, 8, 0x44)
    v = collect(walls, x, y, facing)
    for n in (4, 3, 2, 1):
        i = n - 1
        dv = lambda val: 0x10 if val == 2 else 0   # noqa: E731
        if ("L", n) in v:
            blit(wb, i + 4 + dv(v[("L", n)]), BL_X[i], BL_Y[i])
        if ("LO", n) in v:
            blit(wb, BO_IMG[i] + dv(v[("LO", n)]), BO_X[i], BO_Y[i])
        if ("R", n) in v:
            blit(wb, i + 8 + dv(v[("R", n)]), CL_X[i], CL_Y[i])
        if ("RO", n) in v:
            blit(wb, CO_IMG[i] + dv(v[("RO", n)]), CO_X[i], CO_Y[i])
        if ("F", n) in v:
            blit(wb, i + dv(v[("F", n)]), A_X[i], A_Y[i])
    return canvas


if __name__ == "__main__":
    m, x, y, f, out = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), sys.argv[4], sys.argv[5]
    st = (sys.argv[6] if len(sys.argv) > 6 else "town").upper()
    c = render(GAME, m, x, y, f, st)
    g.write_png(out, 320, 200, c, g.EGA)

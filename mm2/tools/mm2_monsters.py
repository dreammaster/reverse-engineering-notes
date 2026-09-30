"""
MONSTERS.16 pictures.  Format derived from EGA.DRV fn 16h (0x1233) and its piece decoder (0x1422).

MONSTERS.16 = 75 x u32 offsets (0 = unused id) to LZW banks (u32 size + LZW, see mm2_lzw).
Bank:   u16 count, u16 piece_offset[count], animation table, pieces.
Frames: 96x96 pixels, 4 bpp (48 bytes/row).  Frame 0 = piece 0 painted over the screen background;
        frame k (k >= 1) = frame 0 with piece k painted over it.  (The driver captures the
        96x96 screen area first, so "transparent" pixels show whatever was drawn behind.)
Piece:  u8 x, u8 y, u8 width, u8 height  (drawn at pixel (x+4, y+6) in the frame), then a byte
        stream of runs: high nibble = run length - 1, low nibble = colour code; code 5 = leave
        the underlying pixel, otherwise colour = CODE_TO_COLOUR[code]; runs wrap row by row.
Animation table (after the offsets): sequences of (frame, delay) byte pairs, each ended by FFh,
        the whole table ended by another FFh (meaning of the leading word not traced).

    python mm2_monsters.py OUTDIR      # contact sheet per monster bank
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_gfx import EGA, bank, write_png  # noqa: E402
from mm2_layout import DEFAULT_GAME_DIR  # noqa: E402

CODE_TO_COLOUR = [0, 1, 2, 9, 6, 8, 10, 3, 4, 5, 7, 11, 12, 13, 14, 15]   # EGA.DRV:121B
TRANSPARENT = 5
W = H = 96


def banks(game_dir=DEFAULT_GAME_DIR, name="MONSTERS.16"):
    d = open(os.path.join(game_dir, name), "rb").read()
    offs = struct.unpack_from("<75I", d, 0)
    nz = sorted(o for o in offs if o) + [len(d)]
    for idx, o in enumerate(offs):
        if o:
            nxt = min(x for x in nz if x > o)
            yield idx, bank(d[o:nxt])


def paint(frame, b, off):
    x, y, w, h = b[off], b[off + 1], b[off + 2], b[off + 3]
    px, py = x + 4, y + 6
    p = off + 4
    col = row = 0
    while row < h and p < len(b):
        v = b[p]
        p += 1
        n, code = (v >> 4) + 1, v & 15
        for _ in range(n):
            if row >= h:
                break
            if code != TRANSPARENT and 0 <= px + col < W and 0 <= py + row < H:
                frame[py + row][px + col] = CODE_TO_COLOUR[code]
            col += 1
            if col == w:
                col, row = 0, row + 1
    return p


def frames(b, background=0):
    n = struct.unpack_from("<H", b, 0)[0]
    offs = struct.unpack_from("<%dH" % n, b, 2)
    base = [[background] * W for _ in range(H)]
    paint(base, b, offs[0])
    out = [base]
    for k in range(1, n):
        f = [row[:] for row in base]
        paint(f, b, offs[k])
        out.append(f)
    return out


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for idx, b in banks():
        fr = frames(b)
        cols = min(len(fr), 6)
        rows = (len(fr) + cols - 1) // cols
        sheet = [[0] * (cols * (W + 2)) for _ in range(rows * (H + 2))]
        for i, f in enumerate(fr):
            ox, oy = (i % cols) * (W + 2), (i // cols) * (H + 2)
            for y in range(H):
                sheet[oy + y][ox:ox + W] = f[y]
        write_png(os.path.join(out, "monster%02d.png" % idx), len(sheet[0]), len(sheet), sheet, EGA)
        print(idx, len(fr))


if __name__ == "__main__":
    main()

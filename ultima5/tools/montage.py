"""Tile the message-log area of several screenshots into one PNG for quick reading.

    python tools/montage.py out.png shot1.png shot2.png ...   [--box x0,y0,x1,y1] [--cols N]
"""
import argparse
import struct
import sys
import zlib

import smoke_test as t


def write_png(path, w, h, rows):
    def chunk(typ, data):
        c = struct.pack(">I", len(data)) + typ + data
        return c + struct.pack(">I", zlib.crc32(typ + data) & 0xFFFFFFFF)
    raw = b"".join(b"\x00" + r for r in rows)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                           chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("shots", nargs="+")
    ap.add_argument("--box", default="384,170,640,400")
    ap.add_argument("--cols", type=int, default=4)
    a = ap.parse_args()
    x0, y0, x1, y1 = map(int, a.box.split(","))
    bw, bh = x1 - x0, y1 - y0
    cells = []
    for s in a.shots:
        w, h, raw = t.load_png_rgb(s)
        st = 1 + w * 3
        cells.append([raw[y * st + 1 + x0 * 3:y * st + 1 + x1 * 3] for y in range(y0, y1)])
    cols = min(a.cols, len(cells))
    nrows = (len(cells) + cols - 1) // cols
    rows = []
    for r in range(nrows):
        for y in range(bh):
            line = b""
            for c in range(cols):
                i = r * cols + c
                line += (cells[i][y] if i < len(cells) else bytes(bw * 3)) + (b"\x55\x55\x55" * 2)
            rows.append(line)
    write_png(a.out, cols * (bw + 2), len(rows), rows)


if __name__ == "__main__":
    main()

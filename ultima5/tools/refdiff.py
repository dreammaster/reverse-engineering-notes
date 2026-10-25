"""Compare one of our screenshots with a reference screenshot (e.g. a DOSBox window capture).

    python tools/refdiff.py REF.png OURS.png [--ox N --oy N] [--out diff.png]

Decodes 8-bit RGB/RGBA PNGs, finds the offset of the 640x400 game area in REF (searches near the top-left), reports the
fraction of differing pixels and the bounding boxes of the differing 32x32 cells, and writes an optional diff image.
"""
import argparse
import struct
import zlib


def dec(p):
    data = open(p, "rb").read()
    pos, idat = 8, b""
    while pos < len(data):
        n, t = struct.unpack(">I4s", data[pos:pos + 8])
        b = data[pos + 8:pos + 8 + n]
        if t == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", b[:10])
        elif t == b"IDAT":
            idat += b
        pos += 12 + n
    raw = zlib.decompress(idat)
    bpp = {2: 3, 6: 4}[ct]
    st = w * bpp
    rows, prev, o = [], bytearray(st), 0
    for y in range(h):
        f = raw[o]
        line = bytearray(raw[o + 1:o + 1 + st])
        o += 1 + st
        for i in range(st):
            a = line[i - bpp] if i >= bpp else 0
            b_ = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b_) & 255
            elif f == 3:
                line[i] = (line[i] + ((a + b_) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b_ - c), abs(a - c), abs(a + b_ - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else (b_ if pb <= pc else c))) & 255
        rows.append(bytes(line))
        prev = line
    return w, h, bpp, rows


def pix(img, x, y):
    w, h, b, r = img
    return r[y][x * b:x * b + 3]


def region_diff(ref, our, ox, oy, box):
    """Number of differing pixels of OUR inside box=(x0,y0,x1,y1), against REF shifted by (ox,oy)."""
    x0, y0, x1, y1 = box
    return sum(pix(ref, x + ox, y + oy) != pix(our, x, y) for y in range(y0, y1) for x in range(x0, x1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ref")
    ap.add_argument("ours")
    ap.add_argument("--ox", type=int)
    ap.add_argument("--oy", type=int)
    a = ap.parse_args()
    ref, our = dec(a.ref), dec(a.ours)
    W, H = our[0], our[1]
    if a.ox is None:
        best = None
        for oy in range(0, 40):
            for ox in range(0, 6):
                if ox + W > ref[0] or oy + H > ref[1]:
                    continue
                d = sum(pix(ref, x + ox, y + oy) != pix(our, x, y) for y in range(0, 360, 4) for x in range(10, 370, 4))
                if best is None or d < best[0]:
                    best = (d, ox, oy)
        _, a.ox, a.oy = best
    diff = 0
    cells = {}
    for y in range(H):
        if y + a.oy >= ref[1]:
            break
        for x in range(W):
            if x + a.ox >= ref[0]:
                break
            if pix(ref, x + a.ox, y + a.oy) != pix(our, x, y):
                diff += 1
                cells[(x // 32, y // 32)] = cells.get((x // 32, y // 32), 0) + 1
    print("offset (%d,%d): %d of %d pixels differ (%.2f%%)" % (a.ox, a.oy, diff, W * H, 100.0 * diff / (W * H)))
    for (cx, cy), n in sorted(cells.items(), key=lambda kv: -kv[1])[:15]:
        print("  cell x=%d..%d y=%d..%d: %d px" % (cx * 32, cx * 32 + 31, cy * 32, cy * 32 + 31, n))


if __name__ == "__main__":
    main()

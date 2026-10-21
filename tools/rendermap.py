import struct, sys, os
from fodlib import *
from pngwrite import write_png
PAL6 = [(0,0,0),(0,0,0x27),(3,0x25,3),(0,0x1B,0x1B),(0x20,0,0),(0x1B,0,0x1B),(0x20,0x1B,5),(0x26,0x26,0x26),
        (0x10,0x10,0x10),(0,0,0x38),(0,0x3F,0),(0,0x3F,0x3F),(0x3A,0x20,0x10),(0x3F,0,0x3F),(0x3F,0x3F,0),(0x3F,0x3F,0x3F)]
PAL = [tuple(c * 255 // 63 for c in p) for p in PAL6]
tiles = lzss(open(os.path.join(GAME, "TILES"), "rb").read())

def tile_rgb(idx):
    t = tiles[idx * 128: idx * 128 + 128]
    rows = []
    for y in range(16):
        row = bytearray()
        for b in t[y * 8:(y + 1) * 8]:
            row += bytes(PAL[b >> 4]) + bytes(PAL[b & 15])
        rows.append(bytes(row))
    return rows

def render(name, idx, out):
    m = chunk(name, 28, idx)
    w, h = struct.unpack_from("<HH", m, 0)
    img = [bytearray() for _ in range(h * 16)]
    for y in range(h):
        for x in range(w):
            tid = struct.unpack_from("<H", m, 0x414 + (y * w + x) * 8)[0] & 0x7FF
            rows = tile_rgb(tid)
            for r in range(16):
                img[y * 16 + r] += rows[r]
    write_png(out, w * 16, h * 16, b"".join(bytes(r) for r in img))
    print("wrote", out, w * 16, h * 16)

if __name__ == "__main__":
    render(sys.argv[1], int(sys.argv[2]), sys.argv[3])

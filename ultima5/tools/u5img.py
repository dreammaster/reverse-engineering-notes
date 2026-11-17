"""Ultima V picture files (*.4 = CGA 2 bits/pixel, *.16 = EGA 4 bits/pixel): decoder and PNG writer.

python tools/u5img.py tiles FILE.4|FILE.16 out.png          sheet of the 512 tiles (32 per row)
python tools/u5img.py image FILE.4|FILE.16 INDEX out.png     one picture (and its mask, if any) of a picture file

See docs/picture_formats.md for the formats.
"""
import struct
import sys
import zlib

from u5lzw import lzw_decompress

CGA_PALETTE = [(0, 0, 0), (0, 170, 170), (170, 0, 170), (170, 170, 170)]   # BIOS mode 4, palette 1 (cyan/magenta/white), black background
EGA_PALETTE = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
               (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255), (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255)]


def write_png(path, w, h, rgb):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += bytes(c for px in rgb[y * w:(y + 1) * w] for c in px)

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                           chunk(b"IDAT", zlib.compress(bytes(raw))) + chunk(b"IEND", b""))


def load(path):
    return lzw_decompress(open(path, "rb").read())


def tile_pixels(data, idx, cga):
    """16x16 tile -> 256 palette indices.  CGA: 64 bytes/tile, 4 bytes/row, 4 pixels/byte, leftmost pixel in the top bits.
    EGA: 128 bytes/tile, 8 bytes/row, 2 pixels/byte, leftmost pixel in the high nibble."""
    out = []
    if cga:
        t = data[idx * 64:(idx + 1) * 64]
        for y in range(16):
            for b in t[y * 4:y * 4 + 4]:
                out += [(b >> 6) & 3, (b >> 4) & 3, (b >> 2) & 3, b & 3]
    else:
        t = data[idx * 128:(idx + 1) * 128]
        for y in range(16):
            for b in t[y * 8:y * 8 + 8]:
                out += [b >> 4, b & 15]
    return out


def picture(data, idx, cga):
    """Picture number idx of a picture file -> (w, h, pixels, mask) ; mask is a list of 0/1 (1 = transparent) or None."""
    n = struct.unpack_from("<H", data, 0)[0]
    if idx >= n:
        raise IndexError("file has %d pictures" % n)
    io, mo = struct.unpack_from("<HH", data, 2 + idx * 4)
    w, h = struct.unpack_from("<HH", data, io)
    px = []
    if cga:
        stride = (w + 3) // 4
        for y in range(h):
            row = data[io + 4 + y * stride:io + 4 + (y + 1) * stride]
            r = []
            for b in row:
                r += [(b >> 6) & 3, (b >> 4) & 3, (b >> 2) & 3, b & 3]
            px += r[:w]
    else:
        stride = ((w + 7) // 8) * 4            # nibble packed (2 pixels per byte, high nibble first), rows padded to a multiple of 4 bytes
        for y in range(h):
            row = data[io + 4 + y * stride:io + 4 + (y + 1) * stride]
            r = []
            for b in row:
                r += [b >> 4, b & 15]
            px += r[:w]
    mask = None
    if mo:
        mw, mh = struct.unpack_from("<HH", data, mo)
        ms = (mw + 7) // 8
        mask = []
        for y in range(mh):
            row = data[mo + 4 + y * ms:mo + 4 + (y + 1) * ms]
            mask += [(row[x >> 3] >> (7 - (x & 7))) & 1 for x in range(mw)]
    return w, h, px, mask


def main():
    cmd, path = sys.argv[1], sys.argv[2]
    cga = path.lower().endswith(".4")
    pal = CGA_PALETTE if cga else EGA_PALETTE
    data = load(path)
    if cmd == "tiles":
        cols = 32
        W, H = cols * 16, 16 * 16
        img = [(40, 40, 40)] * (W * H)
        for t in range(512):
            px = tile_pixels(data, t, cga)
            ox, oy = (t % cols) * 16, (t // cols) * 16
            for i, v in enumerate(px):
                img[(oy + i // 16) * W + ox + i % 16] = pal[v]
        write_png(sys.argv[3], W, H, img)
    else:
        w, h, px, mask = picture(data, int(sys.argv[3]), cga)
        img = []
        for i, v in enumerate(px):
            c = pal[v]
            if mask is not None and mask[i]:
                c = (255, 0, 255)       # transparent
            img.append(c)
        write_png(sys.argv[4], w, h, img)


main()

"""
MM2 image banks (*.16 = 4 bits/pixel, *.4 = 2 bits/pixel).  Format derived from the EGA driver's
"draw image" routine (driver fn 13h, EGA.DRV:0BCA):

    u32 decompressed size, then LZW (mm2_lzw) ->
    bank:  u16 count, then count * { u16 image_offset, u16 mask_offset (0 = none) }
    image: u16 width, u16 height, then rows of ceil(width/2) bytes (16 colours) or ceil(width/4)
           (4 colours), each row padded up to a multiple of 4 bytes; leftmost pixel in the high bits.
    mask:  no header; 1 bit per pixel, rows of ceil(width/8) bytes, same size as its image.

MONSTERS.16/.4 is different: a table of u32 offsets to separately LZW-compressed banks
(not decoded by this script yet).

    python mm2_gfx.py FILE.16 OUTDIR      # write PNG images (and masks) for every entry
    python mm2_gfx.py FILE.16 OUTDIR sheet   # additionally one contact sheet, sheet.png
"""
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_lzw import lzw_decode  # noqa: E402

# default IBM EGA palette / CGA palette 1 (a guess for the .4 files)
EGA = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170), (170, 85, 0),
       (170, 170, 170), (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255), (255, 85, 85),
       (255, 85, 255), (255, 255, 85), (255, 255, 255)]
CGA = [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)]


def bank(blob):
    """Contents of a bank file -> raw bank bytes."""
    size = struct.unpack_from("<I", blob, 0)[0]
    out = lzw_decode(blob[4:])
    assert len(out) == size
    return out


def entries(b):
    n = struct.unpack_from("<H", b, 0)[0]
    return [struct.unpack_from("<HH", b, 2 + 4 * i) for i in range(n)]


def image(b, off, bpp):
    """-> (width, height, rows of palette indices)"""
    w, h = struct.unpack_from("<HH", b, off)
    per = 8 // bpp
    rowb = ((w + per - 1) // per + 3) & ~3
    pix = []
    p = off + 4
    for _y in range(h):
        row = b[p:p + rowb]
        pix.append([(row[x // per] >> (8 - bpp * (x % per + 1))) & ((1 << bpp) - 1) for x in range(w)])
        p += rowb
    return w, h, pix


def mask(b, off, w, h):
    rowb = (w + 7) // 8
    return [[(b[off + y * rowb + x // 8] >> (7 - x % 8)) & 1 for x in range(w)] for y in range(h)]


def write_png(path, w, h, pix, pal):
    raw = b"".join(b"\0" + bytes(c for v in line for c in pal[v]) for line in pix)

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(bytes([0x89]) + b"PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def contact_sheet(b, bpp, pal, width=700):
    """Lay all images out left-to-right, top-to-bottom on one canvas."""
    items = [image(b, io, bpp) for io, _ in entries(b)]
    x = y = rowh = 0
    place = []
    for w, h, pix in items:
        if x + w > width:
            x, y, rowh = 0, y + rowh + 2, 0
        place.append((x, y, w, h, pix))
        x += w + 2
        rowh = max(rowh, h)
    height = y + rowh
    sheet = [[0] * width for _ in range(height)]
    for x, y, w, h, pix in place:
        for j in range(h):
            sheet[y + j][x:x + w] = pix[j]
    return width, height, sheet


def main():
    src, out = sys.argv[1], sys.argv[2]
    bpp = 4 if src.lower().endswith(".16") else 2
    pal = EGA if bpp == 4 else CGA
    b = bank(open(src, "rb").read())
    os.makedirs(out, exist_ok=True)
    for i, (io, mo) in enumerate(entries(b)):
        w, h, pix = image(b, io, bpp)
        write_png(os.path.join(out, "%02d.png" % i), w, h, pix, pal)
        if mo:
            write_png(os.path.join(out, "%02d_mask.png" % i), w, h, mask(b, mo, w, h), [(0, 0, 0), (255, 255, 255)])
        print(i, io, mo, w, h)
    if len(sys.argv) > 3 and sys.argv[3] == "sheet":
        w, h, sheet = contact_sheet(b, bpp, pal)
        write_png(os.path.join(out, "sheet.png"), w, h, sheet, pal)


if __name__ == "__main__":
    main()

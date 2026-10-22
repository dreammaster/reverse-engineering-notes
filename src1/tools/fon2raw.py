"""
fon2raw.py <font.fon> <pixel height> <out.bin> [pad_to_height]

Extract one 8-pixel-wide bitmap font from a Windows raster .FON file as the raw
BIOS-style table the engine expects: 256 glyphs of <height> bytes, one byte per
pixel row, bit 7 = leftmost pixel. Glyphs the font does not have are blank. With
pad_to_height the rows are centred in a taller cell (for example 8x12 -> 8x14).

The original game uses the PC BIOS ROM fonts (8x8 and 8x14), which are not part
of the game files. This is a local development aid that produces stand-ins from
fonts already installed on the machine (e.g. C:\\Windows\\Fonts\\cga80850.fon for
8x8, vga850.fon for 8x12). The output is not committed (src1/fonts/ is ignored);
a ScummVM port would supply its own font.
"""
import struct
import sys


def fonts(path):
    d = open(path, "rb").read()
    ne = struct.unpack_from("<I", d, 0x3C)[0]
    if d[ne:ne + 2] != b"NE":
        raise SystemExit("not a NE .FON file")
    rt = ne + struct.unpack_from("<H", d, ne + 0x24)[0]
    shift = struct.unpack_from("<H", d, rt)[0]
    p = rt + 2
    while True:
        tid = struct.unpack_from("<H", d, p)[0]
        if tid == 0:
            return
        cnt = struct.unpack_from("<H", d, p + 2)[0]
        p += 8
        for _ in range(cnt):
            off, ln = struct.unpack_from("<HH", d, p)
            p += 12
            if tid == 0x8008:
                yield d[off << shift:(off << shift) + (ln << shift)]


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    path, height, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    cell = int(sys.argv[4]) if len(sys.argv) > 4 else height
    for b in fonts(path):
        ver = struct.unpack_from("<H", b, 0)[0]
        width = struct.unpack_from("<H", b, 0x56)[0]
        h = struct.unpack_from("<H", b, 0x58)[0]
        if width != 8 or h != height:
            continue
        first, last = b[0x5F], b[0x60]
        table = 0x94 if ver == 0x300 else 0x76
        entry = 6 if ver == 0x300 else 4
        top = (cell - height) // 2
        raw = bytearray(256 * cell)
        for c in range(first, last + 1):
            e = table + (c - first) * entry
            off = struct.unpack_from("<I" if ver == 0x300 else "<H", b, e + 2)[0]
            for r in range(height):
                raw[c * cell + top + r] = b[off + r]
        open(out, "wb").write(raw)
        print(f"{out}: {256 * cell} bytes (8x{cell} cells from {width}x{h}, chars {first}-{last})")
        return
    raise SystemExit(f"no 8x{height} font in {path}")


main()

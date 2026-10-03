"""
Renders a contact sheet (PNG, no dependencies) of pictures from PICTURES.VGA using the master palette in WORLD.DAT.

    python pic_sheet.py <game 2|3> <category 0-9> <first id> <count> <columns> <out.png> [scale]

Category geometry and file offsets are the ones in src23/pictures.c (offset = base + id * width * height; colour 0xFF is
the transparent key and is drawn magenta).
"""
import struct
import sys
import zlib

DIRS = {
    2: [(318, 198, 0), (210, 105, 944460), (140, 155, 3171510), (190, 110, 7837010), (224, 74, 11222810),
        (224, 62, 11521178), (56, 136, 11687834), (32, 32, 12106714), (16, 16, 12383194), (8, 8, 12513754)],
    3: [(318, 198, 0), (210, 105, 1448172), (140, 155, 4887972), (190, 110, 10746972), (224, 74, 15721172),
        (224, 62, 16185300), (56, 136, 16379732), (32, 32, 16912852), (16, 16, 17097172), (8, 8, 17184212)],
}
PALETTE = {2: 0x8270A, 3: 0x95BDA}


def png(path, width, height, rgb):
    raw = b"".join(b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))


def main():
    game, cat, first, count, cols = (int(x) for x in sys.argv[1:6])
    out = sys.argv[6]
    scale = int(sys.argv[7]) if len(sys.argv) > 7 else 1
    root = "C:/dev/yendor/yendor%d/game/" % game
    w, h, base = DIRS[game][cat]
    world = open(root + "WORLD.DAT", "rb").read()
    pal = [tuple(min(255, v * 4 + v // 16) for v in world[PALETTE[game] + i * 3:PALETTE[game] + i * 3 + 3]) for i in range(256)]
    pal[255] = (255, 0, 255)
    vga = open(root + "PICTURES.VGA", "rb")
    rows = (count + cols - 1) // cols
    cw, ch = (w + 2) * scale, (h + 2) * scale
    sheetW, sheetH = cols * cw, rows * ch
    buf = bytearray(b"\x20" * (sheetW * sheetH * 3))
    for n in range(count):
        vga.seek(base + (first + n) * w * h)
        data = vga.read(w * h)
        if len(data) < w * h:
            break
        ox, oy = (n % cols) * cw, (n // cols) * ch
        for y in range(h):
            for x in range(w):
                r, g, b = pal[data[y * w + x]]
                for sy in range(scale):
                    for sx in range(scale):
                        o = ((oy + y * scale + sy) * sheetW + ox + x * scale + sx) * 3
                        buf[o:o + 3] = bytes((r, g, b))
    png(out, sheetW, sheetH, bytes(buf))
    print("wrote", out, sheetW, "x", sheetH)


main()

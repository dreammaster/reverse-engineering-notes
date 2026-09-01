#!/usr/bin/env python3
"""Decode the LEGACY.DAT software font -- the 8x8 CGA glyph set that the
engine renders ALL in-game text with.

    python decoders/legacy_font.py [C:\\games\\lota\\LEGACY.DAT] [outdir]

What it is
----------
`MENU.EXE`'s `menuStartup` opens LEGACY.DAT and loads three resident
LEGLIB arrays from the region between the 6-byte header and the string
pool at 0x648:

    0x006-0x605  1536 B / 768 words -> array ds:1EC0  (DIM 800)
                 == THE FONT: 96 glyphs (ASCII 0x20..0x7F), 16 bytes each
    0x604-0x627  18 words           -> array ds:1E8E  (DIM(5,2) = 3x6)
                 == the game-speed timing table: 1E8E[speed][phase] -> a
                    message/animation delay, indexed by ds:1AC8 (game
                    speed), min-clamped to 0x3C.  Used by rtm_FE4B.
    0x628-0x647  16 words           -> array ds:1C4E  (DIM(3,3) = 4x4)
                 == the first-person movement table: 1C4E[(key-1)*4 +
                    facing] -> a packed value; v>>6 = turn quadrant,
                    v & 0x3F = step delta.  Used by DUN/MUS doMovement.

(The earlier "command-menu icon bitmaps" guess was wrong -- it is a
font, and 1C4E is the nav table, not icons.)

Font format
-----------
`drawStringInner` -> `sub_28B90` -> `rtm_FE34`: for each character,
    glyph = fontBase + (ord(ch) - 0x20) * 16
then `sub_1FED8` copies the 16 bytes (8 words) to CGA video B800 as a
**field-interleaved 8x8 cell** -- words go to screen scanlines
0,2,4,6 (even field) then 1,3,5,7 (odd field), exactly the engine's
universal tile-cell layout.  CGA mode 4, 2 bpp, palette 1
(black / cyan / magenta / white); colour 0 is the background.
Each glyph cell advances the video pointer by 2 bytes = 8 px.
"""
import struct
import sys
import zlib

HEADER = 6
FONT_OFF = 0x006
GLYPH_BYTES = 16          # 8 words, field-interleaved
N_GLYPHS = 96             # ASCII 0x20 .. 0x7F
FIRST_CH = 0x20

# CGA palette 1, high intensity
PALETTE = [(0x00, 0x00, 0x00), (0x55, 0xFF, 0xFF),
           (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0xFF)]


def glyph_rows(cell):
    """16 bytes field-interleaved -> 8 rows of 8 pixel indices (0..3)."""
    w = struct.unpack("<8H", cell)
    order = [w[0], w[4], w[1], w[5], w[2], w[6], w[3], w[7]]
    rows = []
    for word in order:
        # little-endian in memory: low byte = left 4 px, high byte = right;
        # within a byte the leftmost pixel is in bits 7-6
        px = []
        for byte in (word & 0xFF, word >> 8):
            for shift in (6, 4, 2, 0):
                px.append((byte >> shift) & 3)
        rows.append(px)
    return rows


def glyphs(data):
    base = FONT_OFF
    for i in range(N_GLYPHS):
        yield FIRST_CH + i, data[base + i * GLYPH_BYTES: base + (i + 1) * GLYPH_BYTES]


def write_png(path, pixels):
    h = len(pixels)
    w = len(pixels[0])
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for (r, g, b) in row:
            raw += bytes((r, g, b))
    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload
                + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    open(path, "wb").write(png)


def render_sheet(data, path, cols=16, scale=6, pad=2):
    gl = list(glyphs(data))
    rows = (len(gl) + cols - 1) // cols
    cw, ch = 8 + pad, 8 + pad
    W = cols * cw * scale
    H = rows * ch * scale
    img = [[(20, 20, 30)] * W for _ in range(H)]
    for idx, (code, cell) in enumerate(gl):
        gx = (idx % cols) * cw + pad // 2
        gy = (idx // cols) * ch + pad // 2
        for y, prow in enumerate(glyph_rows(cell)):
            for x, p in enumerate(prow):
                col = PALETTE[p]
                for sy in range(scale):
                    for sx in range(scale):
                        img[(gy + y) * scale + sy][(gx + x) * scale + sx] = col
    write_png(path, img)
    return len(gl), rows


def main():
    args = sys.argv[1:]
    path = args[0] if args else r"C:\games\lota\LEGACY.DAT"
    outdir = args[1] if len(args) > 1 else "."
    data = open(path, "rb").read()

    hdr = struct.unpack_from("<3H", data, 0)
    print(f"{path}")
    print(f"  header words: {hdr}  (w0/2-3+1 = {hdr[0]//2-3+1} font words)")
    print(f"  font: 0x{FONT_OFF:03X}..0x{FONT_OFF + N_GLYPHS*GLYPH_BYTES - 1:03X}"
          f"  {N_GLYPHS} glyphs x {GLYPH_BYTES} B  (ASCII 0x20..0x7F)")

    # ASCII-art preview of a few glyphs
    ramp = " .:#"
    for code in (ord("A"), ord("a"), ord("0"), ord("?"), ord("@")):
        cell = data[FONT_OFF + (code - FIRST_CH) * GLYPH_BYTES:
                    FONT_OFF + (code - FIRST_CH + 1) * GLYPH_BYTES]
        print(f"\n  '{chr(code)}' (0x{code:02X}):")
        for prow in glyph_rows(cell):
            print("    " + "".join(ramp[p] for p in prow))

    sheet = f"{outdir}/legacy_font.png".replace("//", "/")
    n, rows = render_sheet(data, sheet)
    print(f"\n  wrote {sheet}  ({n} glyphs, {rows} rows x 16)")

    # the two companion tables
    t_speed = struct.unpack_from("<18h", data, hdr[0] - 1)
    t_move = struct.unpack_from("<16h", data, hdr[0] - 1 + 36)
    print(f"\n  game-speed timing table (1E8E, 3x6): {list(t_speed)}")
    print(f"  first-person move table  (1C4E, 4x4): {list(t_move)}")


if __name__ == "__main__":
    main()

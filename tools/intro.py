#!/usr/bin/env python3
"""
The title scroll of INTRO.EXE: LOADING2.PC1 background, FONT2.SPR text, FLAME.DAT candle flames.
Everything below was verified pixel for pixel against INTRO.EXE running in tools/dosemu (see docs/formats.md 4, 8).

FONT2.SPR   31 glyphs x 32 bytes; glyph = 8 rows x 4 plane bytes (bit 7 = left); only the top-left 6x6 pixels
            are drawn, opaque (colour 0 = outside the glyph).  Order: A-Z, '.', ',', '?', '-', "'".
            Text is upper-cased; advance 6 px, rows on a 6 px grid; space = no glyph (background shows).
FLAME.DAT   6 frames x 24 rows x 8 bytes (16 pixels, four big-endian plane words per row, ST interleaved);
            drawn as opaque 16x24 blocks (colour 0 = black; this wipes the static flame in LOADING2.PC1).  Left flame at (14, 20), right flame at (296, 20).  Left flame frame advances
            by 2 or 3 (mod 6) per timer tick, right flame = (left + 2) mod 6 (frames 2 and 4 look identical).
Title page: text starts at x = 68; lines at y = 76, 82, 100, 106, 118, 130.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pc1

GLYPH_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?-\''


def load_font(data):
    glyphs = []
    for g in range(31):
        px = bytearray(36)
        for y in range(6):
            pl = data[g * 32 + y * 4:g * 32 + y * 4 + 4]
            for x in range(6):
                px[y * 6 + x] = sum(((pl[p] >> (7 - x)) & 1) << p for p in range(4))
        glyphs.append(bytes(px))
    return glyphs


def load_flames(data):
    frames = []
    for k in range(6):
        px = bytearray(24 * 16)
        for y in range(24):
            ws = struct.unpack('>4H', data[k * 192 + y * 8:k * 192 + y * 8 + 8])
            for b in range(16):
                px[y * 16 + b] = sum(((ws[p] >> (15 - b)) & 1) << p for p in range(4))
        frames.append(bytes(px))
    return frames


def draw_text(canvas, font, x, y, text):
    for i, ch in enumerate(text.upper()):
        k = GLYPH_CHARS.find(ch)
        if k < 0:
            continue
        for gy in range(6):
            for gx in range(6):
                canvas[(y + gy) * 320 + x + 6 * i + gx] = font[k][gy * 6 + gx]


def draw_flame(canvas, frame, x, y):
    for fy in range(24):
        for fx in range(16):
            canvas[(y + fy) * 320 + x + fx] = frame[fy * 16 + fx]   # opaque: wipes the flame baked into the picture


TITLE_LINES = [(76, '          cadaver'), (82, '          -------'), (100, 'copyright nineteen ninety-one'),
               (106, '    the bitmap brothers'), (118, '          -------')]
PUBLISHER = {'cadaver': (130, '  published by imageworks'), 'payoff': (130, '   published by renagade')}


def render_title(root, game='cadaver', left_flame=1):  # root = game directory containing the loose files
    """-> (palette, 64000 indexed pixels)"""
    pal, bg = pc1.decode(open(os.path.join(root, 'LOADING2.PC1'), 'rb').read())
    canvas = bytearray(bg)
    font = load_font(open(os.path.join(root, 'FONT2.SPR'), 'rb').read())
    flames = load_flames(open(os.path.join(root, 'FLAME.DAT'), 'rb').read())
    for y, t in TITLE_LINES + [PUBLISHER[game]]:
        draw_text(canvas, font, 68, y, t)
    draw_flame(canvas, flames[left_flame % 6], 14, 20)
    draw_flame(canvas, flames[(left_flame + 2) % 6], 296, 20)
    return pal, bytes(canvas)


if __name__ == '__main__':
    from PIL import Image
    game = sys.argv[1] if len(sys.argv) > 1 else 'cadaver1'
    pal, px = render_title(game, 'payoff' if game.endswith('2') else 'cadaver', int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    im = Image.frombytes('P', (320, 200), px)
    im.putpalette([c for rgb in pal for c in rgb])
    im.save(sys.argv[2] if len(sys.argv) > 2 else 'intro.png')

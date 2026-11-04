#!/usr/bin/env python3
"""
Decoder for PCMAN.DAT (the player's sprite set).  See docs/formats.md section 6.2.

The unpacked file is two raw VRAM plane dumps loaded at A000h:A000h of planes 2 and 3
(game routine CAD.EXE 1000:8D86 reads sprites back out of that area):
    bytes [0, 24576)      plane 2 area      -> sprites 0..27
    bytes [24576, 41936)  plane 3 area      -> sprites 28..47
Each area starts with the directory: u16le paragraph offsets, 48 entries (the sprite
directory occupies the first 6 paragraphs), the directory word's high byte >= 6 selects
the plane 3 area (and is reduced by 6).  Sprite data for entry `o` is at byte (o+6)*16
of its area:
    i16 hot_x, i16 hot_y, u16 w, u16 h        (w in 16-pixel words)
    then 5 bitplanes, each h rows of 2*w bytes (MSB = leftmost pixel), padded to 16 bytes:
        plane 0  = transparency mask (set = opaque; equals the OR of the colour planes)
        planes 1-4 = colour bits 0-3
Palette: 16 Atari-ST words (see PALETTE_ST), also present in every LEVn.MP1.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm

# found in CAD.EXE (1F17:F8C2) and in all LEVn.MP1
PALETTE_ST = [0x0000, 0x0243, 0x0400, 0x0032, 0x0310, 0x0420, 0x0111, 0x0775,
              0x0555, 0x0200, 0x0752, 0x0221, 0x0333, 0x0444, 0x0531, 0x0642]


def palette_rgb():
    return [(((w >> 8) & 7) * 255 // 7, ((w >> 4) & 7) * 255 // 7, (w & 7) * 255 // 7) for w in PALETTE_ST]


def load_sprites(data):
    areas = [data[:24576], data[24576:]]
    dirw = struct.unpack('<48H', data[:96])
    sprites = []
    for off in dirw:
        area = areas[1] if (off >> 8) >= 6 else areas[0]
        o = off - 0x600 if (off >> 8) >= 6 else off
        pos = (o + 6) * 16
        hx, hy, w, h = struct.unpack('<hhHH', area[pos:pos + 8])
        bw = 2 * w
        planes = [area[pos + 8 + p * h * bw: pos + 8 + (p + 1) * h * bw] for p in range(5)]
        pix = bytearray(w * 16 * h)  # 0xFF = transparent
        for y in range(h):
            for x in range(w * 16):
                i, sh = y * bw + x // 8, 7 - x % 8
                if (planes[0][i] >> sh) & 1:
                    pix[y * w * 16 + x] = sum(((planes[1 + p][i] >> sh) & 1) << p for p in range(4))
                else:
                    pix[y * w * 16 + x] = 0xFF
        sprites.append(dict(hot=(hx, hy), w=w * 16, h=h, pixels=bytes(pix)))
    return sprites


if __name__ == '__main__':
    from PIL import Image
    spr = load_sprites(mjm.mjm_unpack(open(sys.argv[1], 'rb').read()))
    pal = palette_rgb()
    cols = 12
    W = max(s['w'] for s in spr) + 4
    H = max(s['h'] for s in spr) + 4
    sheet = Image.new('RGB', (cols * W, ((len(spr) + cols - 1) // cols) * H), (60, 60, 90))
    for n, s in enumerate(spr):
        for y in range(s['h']):
            for x in range(s['w']):
                c = s['pixels'][y * s['w'] + x]
                if c != 0xFF:
                    sheet.putpixel(((n % cols) * W + x + 2, (n // cols) * H + y + 2), pal[c])
    sheet = sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST)
    sheet.save(sys.argv[2])
    print(len(spr), 'sprites ->', sys.argv[2])

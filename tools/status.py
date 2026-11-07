#!/usr/bin/env python3
"""
Status text in the screen frame's lower panel (docs/formats.md section 6.3a): the day counter and the room name.

CAD.EXE 1000:DD40 (room_draw_name) builds the room name in a buffer at ds:7EC and draws it centred in a 108-pixel box at
x=202, y=186; 1000:DDB5 clears a box and writes "DAY n" centred in a 60-pixel box at x=252, y=157.  Characters are drawn by
1000:8776 (text_draw): every character advances 7 pixels (a space is just an advance), 'A'..'Z' use glyph c-'A', everything
else below 'A' goes through a 32-byte table at ds:FA40 (index c-0x21).  A glyph is 6 rows of 4 plane bytes (ANDATA block 4,
24 bytes per glyph, 46 glyphs) and acts like a transparent sprite: pixels where any plane bit is set replace the screen.
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

GLYPH_H = 6
ADVANCE = 7
# ds:FA40 + (c - 0x21) for c in '!'..'@'
PUNCT = bytes.fromhex('00 2a 00 00 27 00 00 00 00 00 00 25 28 24 26 1a 1b 1c 1d 1e 1f 20 21 22 23 00 00 2b 2c 2d 29 00')
NAME_BOX = (202, 186, 108)         # x, y, width of the centring box for the room name
DAY_BOX = (252, 157, 60)           # x, y, width; the box is 7 rows high and is cleared to colour 0 first
NAME_MAX = 15                      # 1000:DD6B  mov byte ptr [di+0Fh], 0


def glyph_index(ch):
    """Glyph number of a character, None for a space (advance only)."""
    c = ord(ch)
    if c == 0x20:
        return None
    if c >= 0x41:
        return c - 0x41
    return PUNCT[c - 0x21]


def text_width(s):
    return ADVANCE * len(s)


def draw_string(canvas, x, y, s, font):
    """Draw s with its left edge at x (canvas: 320 x 200 bytearray of colour indices)."""
    for ch in s:
        g = glyph_index(ch)
        if g is not None:
            for r in range(GLYPH_H):
                p = font[g * 24 + r * 4: g * 24 + r * 4 + 4]
                for b in range(8):
                    col = sum(((p[k] >> (7 - b)) & 1) << k for k in range(4))
                    xx = x + b
                    if col and 0 <= xx < 320 and 0 <= y + r < 200:
                        canvas[(y + r) * 320 + xx] = col
        x += ADVANCE


def centred_x(box, s):
    return box[0] + ((box[2] - text_width(s)) >> 1)


def draw_centred(canvas, box, s, font):
    draw_string(canvas, centred_x(box, s), box[1], s, font)


def room_name_string(room_rec, name_table, names, prefix_first=True):
    """Name of a room: [prefix] + ' ' + base (1000:DD40).  room_rec = file bytes of the MP2 section-0 record,
    name_table = 20 big-endian words from the MP1 trailer, names = ENGn.LNG strings.  The word at +0x18 is the prefix string
    id (0xFFFF = none), the byte at +0x1A indexes name_table.  The separating space is added even when the prefix is empty."""
    pre = struct.unpack('>H', room_rec[0x18:0x1A])[0]
    prefix = names[pre] if not pre & 0x8000 and pre < len(names) else ''
    i = room_rec[0x1A]
    base = names[name_table[i]] if not i & 0x80 and i < 20 and name_table[i] < len(names) else ''
    s = prefix + ' ' + base if prefix_first else base + ' ' + prefix
    return s[:NAME_MAX]


def name_table_of(mp1):
    """The 20-word room-name table lives 1630 bytes into the 1790-byte trailer of the unpacked LEVn.MP1."""
    o = len(mp1) - 1790 + 1630
    return struct.unpack('>20H', mp1[o:o + 40])


def draw_status(canvas, font, room_rec, name_table, names, day=1):
    """Draw "DAY n" and the room name; DAY's box is cleared first."""
    x, y, w = DAY_BOX
    for r in range(7):
        for c in range(w):
            canvas[(y + r) * 320 + x + c] = 0
    draw_centred(canvas, DAY_BOX, 'DAY %d' % day, font)
    draw_centred(canvas, NAME_BOX, room_name_string(room_rec, name_table, names), font)
    return canvas

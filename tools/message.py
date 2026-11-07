#!/usr/bin/env python3
"""
Message scrolls (parchment boxes with text), CAD.EXE 1000:5DCB (layout), 1000:8A87 (box), 1000:8714 (centred text).  docs/formats.md 6.7a.

A message is word-wrapped to `w` columns (w = 12 grown until len(text) <= 4w + (w+1) + (w+2) + ...), drawn as a box of 8x8 tiles from ANDATA block 2
(opaque), then the lines in the block-3 font (5 rows of 4 plane bytes, 20 bytes per glyph; the status line uses block 4) with a 6-pixel advance, centred, 6 pixel line pitch.  Slot s (0..3) puts the box at x = 32 + 16 s, y = 20 + 16 s.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import status

# tile numbers of ds:FA1A (ANDATA block 2, 32 bytes each = 8 rows x 4 plane bytes)
TOP = ((0, 1, 2), (5, 0x0E, 0x0C))
LEFT = (6, 7, 7, 8, 8, 8, 8, 8)          # random variant per row (ds:FA20)
RIGHT = (0x0B, 0x0A, 0x0A, 9, 9, 9, 9, 9)  # ds:FA28
FILL = 0x0D
BOTTOM = ((0x10, 0x0F, 0x11), (3, 1, 4))
ADV = 6


def columns(n, w=12):
    """Width in characters (1000:5DD9..5DFC)."""
    dx = n - 4 * w
    if dx >= 0:
        while True:
            w += 1
            dx -= w
            if dx <= 0:
                break
    return w


def wrap(text, w):
    """Lines of the message (1000:5E10..5E4A): words are added while they fit in w columns; a word longer than a line is cut at w."""
    lines = []
    pos = 0
    n = len(text)
    while True:
        bx = 0
        bp = w
        scan = pos
        done = False
        while True:
            # sub_15EE3: advance over one word
            while scan < n and text[scan] != ' ':
                scan += 1
                bx += 1
            if scan >= n:
                done = True                      # 6F4 = 1
            else:
                scan += 1                        # skip the space
            if bx > w:
                done = False
                break
            bp = bx
            bx += 1
            if done:
                break
        lines.append(text[pos:pos + bp])
        pos += bp
        if pos < n and text[pos] == ' ':
            pos += 1
        if done:
            return lines
        if pos >= n:
            return lines


def layout(text, cl=12):
    text += ' '                  # the game's string buffer has a trailing space (it counts for the width and the centring)
    w = columns(len(text), cl)
    lines = wrap(text, w)
    return w, lines


def tile_blit(canvas, x, y, tiles, idx):
    t = tiles[idx * 32:idx * 32 + 32]
    for r in range(8):
        p = t[r * 4:r * 4 + 4]
        for b in range(8):
            col = sum(((p[k] >> (7 - b)) & 1) << k for k in range(4))
            if 0 <= x + b < 320 and 0 <= y + r < 200:
                canvas[(y + r) * 320 + x + b] = col


def draw_box(canvas, x, y, fill, rows, tiles, rnd=None):
    """Box with `fill` columns and `rows` middle rows; rnd() -> 0..7 gives the left/right tile variants."""
    rnd = rnd or (lambda: 0)
    def strip(y, a, b, c):
        tile_blit(canvas, x, y, tiles, a)
        for i in range(fill):
            tile_blit(canvas, x + 8 + 8 * i, y, tiles, b)
        tile_blit(canvas, x + 8 + 8 * fill, y, tiles, c)
    for t in TOP:
        strip(y, *t); y += 8
    for _ in range(rows):
        tile_blit(canvas, x, y, tiles, LEFT[rnd()])
        for i in range(fill):
            tile_blit(canvas, x + 8 + 8 * i, y, tiles, FILL)
        tile_blit(canvas, x + 8 + 8 * fill, y, tiles, RIGHT[rnd()])
        y += 8
    for t in BOTTOM:
        strip(y, *t); y += 8


def draw_text_centred(canvas, x, y, width, s, font):
    xx = x + ((width - ADV * len(s)) >> 1)
    for ch in s:
        g = status.glyph_index(ch)
        if g is not None:
            for r in range(5):
                p = font[g * 20 + r * 4: g * 20 + r * 4 + 4]
                for b in range(8):
                    col = sum(((p[k] >> (7 - b)) & 1) << k for k in range(4))
                    if col and 0 <= xx + b < 320 and 0 <= y + r < 200:
                        canvas[(y + r) * 320 + xx + b] = col
        xx += ADV


def draw_message(canvas, text, slot, tiles, font, rnd=None):
    w, lines = layout(text)
    fill = (w * 6) // 8
    rows = (6 * len(lines)) // 8
    x, y = 0x20 + 16 * slot, 0x14 + 16 * slot
    draw_box(canvas, x, y, fill, rows, tiles, rnd)
    width = 8 * (fill + 2)
    ty = y + 13
    for ln in lines:
        draw_text_centred(canvas, x, ty, width, ln, font)
        ty += 6
    return w, lines, fill, rows

"""
Reader for Shannara .FNT bitmap fonts (from the retail set_font loader).  Layout (verified on all 13 retail fonts: the size formula matches exactly, three 8-bit fonts have a few trailing bytes the engine does not read):

  10 header bytes: [0] ? (1)  [1] bits per pixel (1 = mono, 8 = colour font: one byte per pixel, 0xFE = transparent)  [2] bytesPerRow  [3] firstChar  [4] lastChar  [5] lineSpacing  [6] charHeight
                   [7] charWidth (>= 0 fixed, 0xFF = proportional: width table follows)  [8] charSpacing (>= 0 flat, 0xFF = per-char table follows)  [9] ?
  if charWidth < 0:   u8 widths[lastChar + 1]        (indexed by character code)
  if charSpacing < 0: u8 spacing[lastChar + 1]       (packed nibbles added to the width in the engine)
  glyphs: (lastChar - firstChar + 1) * bytesPerRow * charHeight bytes, 1 bpp, MSB first, bytesPerRow bytes per glyph row.

    python fnt_read.py SHAN001.FNT "Sample text" out.png
"""
import struct
import sys
import zlib


class Font:
    def __init__(self, data):
        h = data
        self.b0, self.b1, self.bpr, self.first, self.last, self.linesp, self.height, self.cw, self.cs, self.b9 = h[:10]
        p = 10
        n = self.last + 1
        self.widths = None
        self.spacing = None
        if self.cw >= 0x80:
            self.widths = data[p:p + n]
            p += n
        if self.cs >= 0x80:
            self.spacing = data[p:p + n]
            p += n
        self.count = self.last - self.first + 1
        self.glyph_size = self.bpr * self.height
        self.glyphs = data[p:p + self.count * self.glyph_size]
        self.expected = p + self.count * self.glyph_size
        self.size = len(data)

    def glyph(self, ch):
        if not self.first <= ch <= self.last:
            return None
        o = (ch - self.first) * self.glyph_size
        return self.glyphs[o:o + self.glyph_size]

    def width(self, ch):
        return self.widths[ch] if self.widths else self.cw

    def render(self, text, scale=2):
        W = sum(max(self.width(ord(c)), 1) + 1 for c in text) + 2
        rows = [[0] * W for _ in range(self.height)]
        x = 1
        for c in text:
            g = self.glyph(ord(c))
            if g:
                for y in range(self.height):
                    for xb in range(min(self.width(ord(c)), self.bpr * (1 if self.b1 == 8 else 8))):
                        if self.b1 == 8:
                            v = g[y * self.bpr + xb]
                            if v not in (0, 0xFE):
                                rows[y][x + xb] = 1
                        elif g[y * self.bpr + xb // 8] & (0x80 >> (xb % 8)):
                            rows[y][x + xb] = 1
            x += max(self.width(ord(c)), 1) + 1
        return W, rows


def write_png(path, rows, scale=3):
    h, w = len(rows), len(rows[0])
    raw = b""
    for y in range(h):
        line = b"".join((b"\x00\x00\x00" if rows[y][x] else b"\xff\xff\xff") * scale for x in range(w))
        raw += (b"\x00" + line) * scale

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


if __name__ == "__main__":
    f = Font(open(sys.argv[1], "rb").read())
    print("first", f.first, "last", f.last, "bpr", f.bpr, "height", f.height, "cw", f.cw, "cs", f.cs, "size", f.size, "expected", f.expected, "OK" if f.size == f.expected else ("OK+%d trailing bytes" % (f.size - f.expected) if f.size > f.expected else "TOO SHORT"))
    if len(sys.argv) >= 4:
        w, rows = f.render(sys.argv[2])
        write_png(sys.argv[3], rows)

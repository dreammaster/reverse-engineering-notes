"""
writeChar (yendor2.asm:46473) draws 6x6 glyphs: glyph = (char - 0x20) * 6 bytes (one byte per row, bit 7 = leftmost of 6 pixels)
from the font table whose near pointer is the word at DS:0x21A + fontOffset (Chapter 3 DS:0x516); fontOffset is 0, 2, 4 or 6.
Dumps the pointers and 96 glyphs of each (as hex).

    .\run_ida_script.ps1 dump_fonts.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
TABLE = 0x21A
lines = []
for k in range(4):
    ptr = ida_bytes.get_word(DS_BASE + TABLE + 2 * k)
    lines.append(f"font {k} ptr=0x{ptr:X}")
    for c in range(96):
        glyph = bytes(ida_bytes.get_byte(DS_BASE + ptr + c * 6 + i) for i in range(6))
        lines.append(f"  {c:2d} {glyph.hex()}")
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor2\ida_scripts\fonts.txt", "w", encoding="utf-8").write(text)
print(text[:400])

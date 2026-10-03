"""
writeChar (yendor3.asm:46473) draws 6x6 glyphs: glyph = (char - 0x20) * 6 bytes (one byte per row, bit 7 = leftmost of 6 pixels)
from the font table whose near pointer is the word at DS:0x516 + fontOffset (Chapter 2 DS:0x21A); fontOffset is 0, 2, 4 or 6.
Dumps the pointers and 96 glyphs of each (as hex).

    .\run_ida_script.ps1 dump_fonts.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
TABLE = 0x516
lines = []
for k in range(4):
    ptr = ida_bytes.get_word(DS_BASE + TABLE + 2 * k)
    lines.append(f"font {k} ptr=0x{ptr:X}")
    for c in range(96):
        glyph = bytes(ida_bytes.get_byte(DS_BASE + ptr + c * 6 + i) for i in range(6))
        lines.append(f"  {c:2d} {glyph.hex()}")
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\fonts.txt", "w", encoding="utf-8").write(text)
print(text[:400])

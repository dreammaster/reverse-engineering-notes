"""
g_pictureDir (DS:0x782E; Chapter 3 DS:0x7B5C) is a table of picture CATEGORIES, 16-byte entries: +8 width, +0xA height,
+0xC dword file offset of the category's first picture in PICTURES.VGA (InitGraphics fills +0/+2/+4/+6 at run time).
Dumps the ten static entries.

    .
un_ida_script.ps1 dump_picture_dir.py -NoExport
"""
import ida_bytes
import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lines = []
for i in range(10):
    ea = DS_BASE + 0x7B5C + i * 0x10
    w = ida_bytes.get_word(ea + 8)
    h = ida_bytes.get_word(ea + 0xA)
    off = ida_bytes.get_word(ea + 0xC) | (ida_bytes.get_word(ea + 0xE) << 16)
    lines.append(f"{i}: width={w} height={h} offset={off}")
pal = ida_bytes.get_word(DS_BASE + 0xB1B7) | (ida_bytes.get_word(DS_BASE + 0xB1B7 + 2) << 16)
lines.append(f"master palette offset in WORLD.DAT = {pal} (0x{pal:X})")
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\picture_dir.txt", "w", encoding="utf-8").write(text)
print(text)

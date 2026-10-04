"""
DrawMonsterInfoPanel (yendor3.asm:34262) writes NUL-terminated strings at DS:0x7ACA..0x7B1F (Chapter 3: 0x7DFC..0x7E51). Dumps them.

    .\run_ida_script.ps1 dump_monster_panel_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
START, END = 0x7DFC, 0x7E52
data = bytes(ida_bytes.get_byte(DS_BASE + a) for a in range(START, END))
lines = []
pos = 0
for part in data.split(b"\0"):
    lines.append("0x%X %r" % (START + pos, part))
    pos += len(part) + 1
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\monster_panel_strings.txt", "w", encoding="utf-8").write(text)
print(text)

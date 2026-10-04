"""
DrawCharacterStatSheet (yendor3.asm:36860) shows, for a level-1 character, one of four packed strings chosen by the average of the six
attributes (<= 49, <= 52, <= 55, else the fourth). Packed at DS:0x8FA2 (Chapter 3: 0x92C1). Dumps them.

    .\run_ida_script.ps1 dump_newchar_titles.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
ADDR = 0x92C1
data = bytes(ida_bytes.get_byte(DS_BASE + ADDR + i) for i in range(80))
text = repr(data.split(b"\0")[:5])
open(r"C:\dev\yendor\yendor3\ida_scripts\newchar_titles.txt", "w", encoding="utf-8").write(text)
print(text)

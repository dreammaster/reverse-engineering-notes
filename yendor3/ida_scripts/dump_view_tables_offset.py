"""
PreloadMonsterStatsTable (misnamed) reads one 0x499C-byte block of WORLD.DAT into word_3291A's segment: the first-person
view's geometry tables (_val11.._val14, _ptr1.._ptr7 index into it: DrawViewportSprite). The file offset is the dword at
DS:0xB1F3 (Chapter 3 DS:0xB1F3, found via seg133).

    .\run_ida_script.ps1 dump_view_tables_offset.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lo = ida_bytes.get_word(DS_BASE + 0xB1F3)
hi = ida_bytes.get_word(DS_BASE + 0xB1F3 + 2)
off = (hi << 16) | lo
text = f"view tables offset = {off} (0x{off:X}), size 0x499C"
open(r"C:\dev\yendor\yendor3\ida_scripts\view_tables_offset.txt", "w", encoding="utf-8").write(text)
print(text)

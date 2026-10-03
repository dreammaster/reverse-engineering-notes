"""
Chapter 3 sibling: the ambient-music table's WORLD.DAT offset is the dword at DS:0xB193; its record size is the
word at DS:0x5494.

    .\run_ida_script.ps1 dump_music_table_offset.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lo = ida_bytes.get_word(DS_BASE + 0xB193)
hi = ida_bytes.get_word(DS_BASE + 0xB193 + 2)
line = f"ambient music table offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), record size word at 0x5494"
with open(r"C:\dev\yendor\yendor3\ida_scripts\music_table_offset.txt", "w", encoding="utf-8") as f:
    f.write(line)
print(line)

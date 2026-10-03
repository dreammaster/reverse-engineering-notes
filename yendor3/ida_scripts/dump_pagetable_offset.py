"""
Chapter 3 sibling: the per-page table's WORLD.DAT offset is the dword at DS:0xB187.

    .\run_ida_script.ps1 dump_pagetable_offset.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lo = ida_bytes.get_word(DS_BASE + 0xB187)
hi = ida_bytes.get_word(DS_BASE + 0xB187 + 2)
line = f"page table offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), record size 6"
with open(r"C:\dev\yendor\yendor3\ida_scripts\pagetable_offset.txt", "w", encoding="utf-8") as f:
    f.write(line)
print(line)

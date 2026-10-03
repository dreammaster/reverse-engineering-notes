"""
Chapter 3 sibling: the same tables sit at DS:0x30A (pairs) and DS:0x3DC (per-cell lists), with the viewport buffer at 0x708E.

    .\run_ida_script.ps1 dump_visibility_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
words = [ida_bytes.get_word(DS_BASE + 2 * i) for i in range(0x400)]
lines = [" ".join(f"{w:04X}" for w in words[i:i + 16]) for i in range(0, len(words), 16)]
with open(r"C:\dev\yendor\yendor3\ida_scripts\visibility_tables.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

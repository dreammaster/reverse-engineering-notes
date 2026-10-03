"""
Chapter 3 sibling of yendor2's dump_class_start_flags.py: the same 6-row table (class 4-9, two ability
flag words each) is at DS:0xB89D.

    .\run_ida_script.ps1 dump_class_start_flags.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lines = []
for k in range(6):
    words = [ida_bytes.get_word(DS_BASE + 0xB89D + k * 4 + 2 * j) for j in range(2)]
    lines.append(f"class {4 + k}: {words[0]} {words[1]}")
out_path = r"C:\dev\yendor\yendor3\ida_scripts\class_start_flags.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

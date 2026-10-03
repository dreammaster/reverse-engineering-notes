"""
Chapter 3 sibling: RepairItemCommand's chance table is at DS:0x6EAC (same shape).

    .\run_ida_script.ps1 dump_repair_table.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lines = []
for row in range(24):
    words = [ida_bytes.get_word(DS_BASE + 0x6EAC + row * 20 + 2 * k) for k in range(10)]
    lines.append(f"row {row}: " + " ".join(f"{w:3d}" for w in words))
out_path = r"C:\dev\yendor\yendor3\ida_scripts\repair_table.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

"""
Chapter 3 sibling: UseAbilityScroll's 4-entry, 26-byte ability table is at
DS:0x7AF4 (name + 4-byte BCD price at +0x12).

    .\run_ida_script.ps1 dump_ability_scroll_table.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lines = []
for i in range(4):
    ea = DS_BASE + 0x7AF4 + 26 * i
    raw = bytes(ida_bytes.get_byte(ea + k) for k in range(26))
    lines.append(f"{i}: {raw!r} price_bytes={raw[0x12:0x16].hex()}")
out_path = r"C:\dev\yendor\yendor3\ida_scripts\ability_scroll_table.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

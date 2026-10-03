"""
UseAbilityScroll (yendor2.asm:21917) shows the ability a teacher offers from a
4-entry table of 26-byte records at DS:0x77C6 (entry picked by the highest set
bit of the topic's ability mask 0x8000/0x4000/0x2000/0x1000): a name field and,
at +0x12, a 4-byte packed-BCD price. Dumps the raw records.

    .\run_ida_script.ps1 dump_ability_scroll_table.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lines = []
for i in range(4):
    ea = DS_BASE + 0x77C6 + 26 * i
    raw = bytes(ida_bytes.get_byte(ea + k) for k in range(26))
    lines.append(f"{i}: {raw!r} price_bytes={raw[0x12:0x16].hex()}")
out_path = r"C:\dev\yendor\yendor2\ida_scripts\ability_scroll_table.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

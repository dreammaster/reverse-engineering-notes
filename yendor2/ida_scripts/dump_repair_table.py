"""
RepairItemCommand (yendor2.asm:50996) rolls RandomInRange(100) against a pair of
words (low, high) from a table at DS:0x6B7E: 20 bytes per row (row = the item's
"+N"/weapon level), 5 tiers x (low, high) word pairs at +0/+4/+8/+12/+16, the tier
chosen by the repairer's PartyStatRepair (<50, <65, <80, <95, else). Dumps 24 rows.

    .\run_ida_script.ps1 dump_repair_table.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lines = []
for row in range(24):
    words = [ida_bytes.get_word(DS_BASE + 0x6B7E + row * 20 + 2 * k) for k in range(10)]
    lines.append(f"row {row}: " + " ".join(f"{w:3d}" for w in words))
out_path = r"C:\dev\yendor\yendor2\ida_scripts\repair_table.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

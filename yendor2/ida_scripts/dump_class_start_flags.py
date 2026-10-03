"""
ApplySecondaryClassTierFlags (yendor2.asm:37509) maps a spellcasting class (4-9) to two
ability-flag indices (PartyFieldFlagBankCA, SetRecordFlag_CA) from a 6-row table at
DS:0xD213, 4 bytes per row (two words, 0 = none). Dumps the six rows.

    .\run_ida_script.ps1 dump_class_start_flags.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lines = []
for k in range(6):
    words = [ida_bytes.get_word(DS_BASE + 0xD213 + k * 4 + 2 * j) for j in range(2)]
    lines.append(f"class {4 + k}: {words[0]} {words[1]}")
out_path = r"C:\dev\yendor\yendor2\ida_scripts\class_start_flags.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

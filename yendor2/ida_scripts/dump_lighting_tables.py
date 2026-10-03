"""
ComputeAmbientLightingTable (yendor2.asm:41988) reads these static tables from the data segment:
  0x7228  day/night cycle: 32-byte entries (+0 marker, 0xFFFF ends the list; +2 end minute; +0x12 seven shade words)
  0x76CA / 0x772C / 0x773A  seven-word gradients used while a travel flag (word_36C79 bit 4 / 2 / 1) is set
  0x76D8  light-source adjustment: 7 rows x 6 words
  0x6E98  nine 8-byte wall-light entries (+2 compared with word_329F0 = 0x2F ... +3 by facing)
Dumps them as Python-literal text.

    .\run_ida_script.ps1 dump_lighting_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def words(off, n):
    return [ida_bytes.get_word(DS_BASE + off + 2 * i) for i in range(n)]


lines = []
off = 0x7228
entries = []
while True:
    marker = ida_bytes.get_word(DS_BASE + off)
    if marker == 0xFFFF:
        entries.append(("end", words(off, 16)))
        break
    entries.append(("entry", words(off, 16)))
    off += 0x20
    if len(entries) > 40:
        break
lines.append(f"time table entries: {len(entries)}")
for kind, w in entries:
    lines.append(f"  {kind}: {w}")
lines.append(f"gradient 0x76CA: {words(0x76CA, 7)}")
lines.append(f"gradient 0x772C: {words(0x772C, 7)}")
lines.append(f"gradient 0x773A: {words(0x773A, 7)}")
lines.append("adjustment 0x76D8 (7 rows x 6):")
for r in range(7):
    lines.append(f"  {words(0x76D8 + r * 12, 6)}")
lines.append("wall-light entries 0x6E98 (9 x 4 words):")
for r in range(9):
    lines.append(f"  {words(0x6E98 + r * 8, 4)}")
out_path = r"C:\dev\yendor\yendor2\ida_scripts\lighting_tables.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

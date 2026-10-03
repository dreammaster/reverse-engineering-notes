"""
Chapter 3 sibling of yendor2's dump_lighting_tables.py. The layout is the same but the addresses are:
  0x7556 time table (32-byte entries ended by 0xFFFF)   0x79F8 / 0x7A68 gradients (travel flags 0x8000 / 0x2000)
  0x7A06 light adjustment (7 x 6 words)                 0x71C6 nine 8-byte wall-light entries

    .\run_ida_script.ps1 dump_lighting_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def words(off, n):
    return [ida_bytes.get_word(DS_BASE + off + 2 * i) for i in range(n)]


lines = []
off = 0x7556
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
lines.append(f"gradient 0x79F8: {words(0x79F8, 7)}")
lines.append(f"gradient 0x7A68: {words(0x7A68, 7)}")
lines.append("adjustment 0x7A06 (7 rows x 6):")
for r in range(7):
    lines.append(f"  {words(0x7A06 + r * 12, 6)}")
lines.append("wall-light entries 0x71C6 (9 x 4 words):")
for r in range(9):
    lines.append(f"  {words(0x71C6 + r * 8, 4)}")
out_path = r"C:\dev\yendor\yendor3\ida_scripts\lighting_tables.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

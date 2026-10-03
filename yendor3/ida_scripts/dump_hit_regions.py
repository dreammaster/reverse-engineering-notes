"""
HitTestRegionTable (yendor2.asm:23244) scans 10-byte entries (xMin, xMax, yMin, yMax, result), screen pixels inclusive, up to a
0xFFFF terminator. Dumps every table a caller passes it (24 tables).

    .\run_ida_script.ps1 dump_hit_regions.py -NoExport
"""
import ida_bytes
import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
TABLES = [0x5e0c, 0x5e4a, 0x5e7e, 0x5ea8, 0x601c, 0x60aa, 0x614c, 0x63de, 0x643a, 0x6504, 0x6646, 0x6670, 0x66a4, 0x6700, 0x6752, 0x6790, 0x685a, 0x692e, 0x69c6, 0x6ada, 0x6ba4, 0x6c00, 0x6c8e, 0x6ca4]
lines = []
for addr in TABLES:
    lines.append("== table 0x%X" % addr)
    for i in range(48):
        ea = DS_BASE + addr + i * 10
        words = [ida_bytes.get_word(ea + 2 * k) for k in range(5)]
        if words[0] == 0xFFFF:
            break
        lines.append("  %d %d %d %d -> %d" % tuple(words))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\hit_regions.txt", "w", encoding="utf-8").write(text)
print(text[:300])

"""
HitTestRegionTable (yendor2.asm:23244) scans 10-byte entries (xMin, xMax, yMin, yMax, result), screen pixels inclusive, up to a
0xFFFF terminator. Dumps every table a caller passes it (24 tables).

    .\run_ida_script.ps1 dump_hit_regions.py -NoExport
"""
import ida_bytes
DS_BASE = 0x2D860
TABLES = [0x5ac0, 0x5afe, 0x5b32, 0x5b5c, 0x5cd0, 0x5d5e, 0x5e00, 0x6092, 0x60ee, 0x61c2, 0x6304, 0x632e, 0x636c, 0x63c8, 0x641a, 0x6458, 0x6522, 0x6600, 0x6698, 0x67ac, 0x6876, 0x68d2, 0x6960, 0x6976]
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
open(r"C:\dev\yendor\yendor2\ida_scripts\hit_regions.txt", "w", encoding="utf-8").write(text)
print(text[:300])

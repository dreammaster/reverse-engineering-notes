"""
The on-screen direction pad's click regions (the lower right panel; `start` hit-tests this table after a click in region 5 of the main
screen table): 10-byte entries xMin, xMax, yMin, yMax, id at DS:0x6752 (Chapter 2 0x641A). Ids 1-6 are turn left, forward, turn right,
strafe left, backward, strafe right. Dumps the first eight entries.

    .\run_ida_script.ps1 dump_dpad_regions.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
ADDR = 0x6752
lines = []
for i in range(8):
    p = ADDR + 10 * i
    lines.append(repr([ida_bytes.get_word(DS_BASE + p + 2 * k) for k in range(5)]))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\dpad_regions.txt", "w", encoding="utf-8").write(text)
print(text)

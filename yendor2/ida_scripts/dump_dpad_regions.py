"""
The on-screen direction pad's click regions (the lower right panel; `start` hit-tests this table after a click in region 5 of the main
screen table): 10-byte entries xMin, xMax, yMin, yMax, id at DS:0x641A (Chapter 3 0x6752). Ids 1-6 are turn left, forward, turn right,
strafe left, backward, strafe right. Dumps the first eight entries.

    .\run_ida_script.ps1 dump_dpad_regions.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
ADDR = 0x641A
lines = []
for i in range(8):
    p = ADDR + 10 * i
    lines.append(repr([ida_bytes.get_word(DS_BASE + p + 2 * k) for k in range(5)]))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor2\ida_scripts\dpad_regions.txt", "w", encoding="utf-8").write(text)
print(text)

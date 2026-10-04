"""
Strings of the pause / system dialog (RunGameDialog, yendor2.asm:26081; labels at :27037-27152, the animation speed label at :27419):
SAVE, LOAD, DOS, MUSIC, SOUND FX, RETURN, the ANIMATION caption and its blanking filler, NEW GAME and the three speed names.
Chapter 3 addresses. Dumps each NUL-terminated string.

    .
un_ida_script.ps1 dump_game_dialog_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
ADDRESSES = [0x7C20, 0x7C25, 0x7C2A, 0x7C2E, 0x7C34, 0x7C3D, 0x8708, 0x7C8D, 0x8712, 0x871B, 0x8722, 0x8729]
lines = []
for addr in ADDRESSES:
    data = bytearray()
    while len(data) < 40:
        b = ida_bytes.get_byte(DS_BASE + addr + len(data))
        if b == 0:
            break
        data.append(b)
    lines.append("%04X %r" % (addr, bytes(data)))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\game_dialog_strings.txt", "w", encoding="utf-8").write(text)
print(text)

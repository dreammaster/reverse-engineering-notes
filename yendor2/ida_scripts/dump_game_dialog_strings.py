"""
Strings of the pause / system dialog (RunGameDialog, yendor2.asm:26081; labels at :27037-27152, the animation speed label at :27419):
SAVE, LOAD, DOS, MUSIC, SOUND FX, RETURN, the ANIMATION caption and its blanking filler, NEW GAME and the three speed names.
Chapter 2 addresses. Dumps each NUL-terminated string.

    .
un_ida_script.ps1 dump_game_dialog_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
ADDRESSES = [0x78F2, 0x78F7, 0x78FC, 0x7900, 0x7906, 0x790F, 0x83E1, 0x795B, 0x83EB, 0x83F4, 0x83FB, 0x8402]
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
open(r"C:\dev\yendor\yendor2\ida_scripts\game_dialog_strings.txt", "w", encoding="utf-8").write(text)
print(text)

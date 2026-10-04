"""
The clue book SPELL page strings (ShowClueBookSpellDetail yendor3.asm:6091 and its chunk): single labels and the packed label columns / class names. Chapter 3 addresses.

    .un_ida_script.ps1 dump_clue_spell_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
SINGLE = [0x8FBC, 0x8FFD, 0x909A, 0x8FF4, 0x9004, 0x9008, 0x907B, 0x92E1, 0x900C, 0x908C, 0x9093, 0x9014, 0x901E, 0x9020, 0x9030, 0x9043, 0x9051, 0x905F, 0x9073]
PACKED = [(0x8FCD, 2), (0x8FDB, 5), (0x7CD5, 6)]


def read_string(addr):
    data = bytearray()
    while len(data) < 40:
        b = ida_bytes.get_byte(DS_BASE + addr + len(data))
        if b == 0:
            break
        data.append(b)
    return bytes(data)


lines = []
for addr in SINGLE:
    lines.append("%04X %r" % (addr, read_string(addr)))
for addr, count in PACKED:
    p = addr
    for i in range(count):
        s = read_string(p)
        lines.append("%04X[%d] %r" % (addr, i, s))
        p += len(s) + 1
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\clue_spell_strings.txt", "w", encoding="utf-8").write(text)
print(text)

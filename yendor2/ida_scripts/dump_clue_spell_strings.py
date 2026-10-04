"""
The clue book SPELL page strings (ShowClueBookSpellDetail yendor2.asm:6091 and its chunk): single labels and the packed label columns / class names. Chapter 2 addresses.

    .un_ida_script.ps1 dump_clue_spell_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x8C9E, 0x8CE6, 0x8D83, 0x8CDD, 0x8CED, 0x8CF1, 0x8D64, 0x8FC2, 0x8CF5, 0x8D75, 0x8D7C, 0x8CFD, 0x8D07, 0x8D09, 0x8D19, 0x8D2C, 0x8D3A, 0x8D48, 0x8D5C]
PACKED = [(0x8CAF, 3), (0x8CC4, 5), (0x79A3, 6)]


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
open(r"C:\dev\yendor\yendor2\ida_scripts\clue_spell_strings.txt", "w", encoding="utf-8").write(text)
print(text)

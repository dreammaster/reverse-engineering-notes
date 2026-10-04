"""
Strings of the clue book item detail screens (ShowClueBookItemDetail yendor2.asm:5668 and the armor / weapon / healing / duration rows after
it): single labels at the listed addresses, and packed tables (consecutive NUL-terminated strings) for the protection names, the attribute /
skill names and the weapon skill types. Chapter 2 addresses.

    .un_ida_script.ps1 dump_clue_item_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x8A82, 0x8A8E, 0x8A96, 0x8AA2, 0x8AAB, 0x8AB1, 0x8AC1, 0x8ACB, 0x8AD5, 0x8ADA, 0x8ADE, 0x8AE8, 0x8AEC, 0x8AEF, 0x8B45, 0x8B4F, 0x8B57, 0x8B5F, 0x8B66, 0x7C81, 0x7B24]
PACKED = [(0x7B31, 9), (0x7DC7, 27), (0x7E8A, 5)]


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
open(r"C:\dev\yendor\yendor2\ida_scripts\clue_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

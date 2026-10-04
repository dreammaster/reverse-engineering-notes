"""
The alchemy screen status panel strings (DrawAlchemyStatusPanel yendor2.asm:25596): the blank filler at (241, 87), the MAGIC: label and the two ore labels. Chapter 2 addresses.

    .
un_ida_script.ps1 dump_alchemy_panel_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x7955, 0x7B15, 0x7C61, 0x7C6D]
PACKED = []


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
open(r"C:\dev\yendor\yendor2\ida_scripts\alchemy_panel_strings.txt", "w", encoding="utf-8").write(text)
print(text)

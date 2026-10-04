"""
The map location suffix templates (BuildClueLocationSuffix yendor2.asm:31620): " LEVEL x" and " MAP x" whose digit is overwritten by the block record. Chapter 2 addresses.

    .un_ida_script.ps1 dump_location_suffix.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x7916, 0x791F]
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
open(r"C:\dev\yendor\yendor2\ida_scripts\location_suffix.txt", "w", encoding="utf-8").write(text)
print(text)

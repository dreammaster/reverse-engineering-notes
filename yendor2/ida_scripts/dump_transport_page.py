"""
The clue book TRANSPORTATIONS page (ShowClueBookTransportDetail yendor2.asm:6430, DrawTransportDetailRow :6455): the four 26-byte mount records (name, price, uses, time word) and the page labels. Chapter 2 addresses.

    .un_ida_script.ps1 dump_transport_page.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x7FBD, 0x8C21, 0x8C27, 0x8C2D, 0x8C3F, 0x8C45, 0x8C4B, 0x8A6A]
RECORDS = 0x77C6
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
for n in range(4):
    raw = bytes(ida_bytes.get_byte(DS_BASE + RECORDS + 26 * n + k) for k in range(26))
    lines.append("record %d %r" % (n, raw))
for addr in SINGLE:
    lines.append("%04X %r" % (addr, read_string(addr)))
for addr, count in PACKED:
    p = addr
    for i in range(count):
        s = read_string(p)
        lines.append("%04X[%d] %r" % (addr, i, s))
        p += len(s) + 1
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor2\ida_scripts\transport_page.txt", "w", encoding="utf-8").write(text)
print(text)

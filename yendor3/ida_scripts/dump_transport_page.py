"""
The clue book TRANSPORTATIONS page (ShowClueBookTransportDetail yendor3.asm:6430, DrawTransportDetailRow :6455): the four 26-byte mount records (name, price, uses, time word) and the page labels. Chapter 3 addresses.

    .
un_ida_script.ps1 dump_transport_page.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
SINGLE = [0x82EA, 0x8F3F, 0x8F45, 0x8F4B, 0x8F5D, 0x8F63, 0x8F69, 0x8D89]
RECORDS = 0x7AF4
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
open(r"C:\dev\yendor\yendor3\ida_scripts\transport_page.txt", "w", encoding="utf-8").write(text)
print(text)

"""
Strings of the clue book item detail screens (ShowClueBookItemDetail yendor2.asm:5668 and the armor / weapon / healing / duration rows after
it): single labels at the listed addresses, and packed tables (consecutive NUL-terminated strings) for the protection names, the attribute /
skill names and the weapon skill types. Chapter 3 addresses.

    .un_ida_script.ps1 dump_clue_item_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
SINGLE = [0x8DA1, 0x8DAD, 0x8DB5, 0x8DC1, 0x8DCA, 0x8DD0, 0x8DE0, 0x8DEA, 0x8DF4, 0x8DF9, 0x8DFD, 0x8E07, 0x8E0B, 0x8E0E, 0x8E62, 0x8E6C, 0x8E7C, 0x8E83, 0x7FAE, 0x7E56]
PACKED = [(0x7E63, 9), (0x80F4, 27), (0x81B7, 5)]


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
open(r"C:\dev\yendor\yendor3\ida_scripts\clue_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

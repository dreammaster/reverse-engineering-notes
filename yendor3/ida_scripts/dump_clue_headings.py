"""
The clue book's category headings, drawn at the top right of every entry page and list (the second string of DrawMessageBox; ShowClueBook
sets one per category 1-4 and 11-17). Dumps them. Chapter 3 addresses.

    .un_ida_script.ps1 dump_clue_headings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
SINGLE = [0x8B92, 0x8B97, 0x8B69, 0x8B7B, 0x8BE1, 0x8D22, 0x8D44, 0x8D5D, 0x8D6B, 0x8D80, 0x8D99]
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
open(r"C:\dev\yendor\yendor3\ida_scripts\clue_headings.txt", "w", encoding="utf-8").write(text)
print(text)

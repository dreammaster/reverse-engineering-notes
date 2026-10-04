"""
The clue book's category headings, drawn at the top right of every entry page and list (the second string of DrawMessageBox; ShowClueBook
sets one per category 1-4 and 11-17). Dumps them. Chapter 2 addresses.

    .un_ida_script.ps1 dump_clue_headings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
SINGLE = [0x8871, 0x8876, 0x8848, 0x885A, 0x88C0, 0x8A01, 0x8A21, 0x8A3F, 0x8A54, 0x8A5C, 0x8A7A]
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
open(r"C:\dev\yendor\yendor2\ida_scripts\clue_headings.txt", "w", encoding="utf-8").write(text)
print(text)

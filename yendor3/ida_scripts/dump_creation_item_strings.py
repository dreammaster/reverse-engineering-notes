"""
Strings of the character-creation screens (ShowCharacterInventory yendor3.asm:36150, ShowCharacterStats :36743, EditCharacterName :36518,
ShowCharacterSummary :37176, DrawQuitOrReturnLabel :37632). Dumps the NUL-terminated string at each address (Chapter 3 addresses here;
the Chapter 2 copy of this script uses that game's).

    .\run_ida_script.ps1 dump_creation_item_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
ADDRESSES = [0x7C94, 0x7C92, 0x7D17, 0x7DB6, 0x7D43, 0x7D4D, 0x7D54, 0x7D5A, 0x7D6A, 0x7D70, 0x7D7F, 0x7D8E, 0x8863, 0x8899, 0x88A9]
lines = []
for addr in ADDRESSES:
    data = bytearray()
    while len(data) < 40:
        b = ida_bytes.get_byte(DS_BASE + addr + len(data))
        if b == 0:
            break
        data.append(b)
    lines.append("%04X %r" % (addr, bytes(data)))
text = "\n".join(lines)
open(r"C:\dev\yendor\yendor3\ida_scripts\creation_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

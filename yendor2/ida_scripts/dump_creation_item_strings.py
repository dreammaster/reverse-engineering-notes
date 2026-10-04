"""
Strings of the character-creation screens (ShowCharacterInventory yendor2.asm:36150, ShowCharacterStats :36743, EditCharacterName :36518,
ShowCharacterSummary :37176, DrawQuitOrReturnLabel :37632). Dumps the NUL-terminated string at each address (Chapter 2 addresses here;
the Chapter 3 copy of this script uses that game's).

    .\run_ida_script.ps1 dump_creation_item_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
ADDRESSES = [0x7962, 0x7960, 0x79E5, 0x7A84, 0x7A11, 0x7A1B, 0x7A22, 0x7A28, 0x7A38, 0x7A3E, 0x7A4D, 0x7A5C, 0x853C, 0x8572, 0x8582]
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
open(r"C:\dev\yendor\yendor2\ida_scripts\creation_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

"""
Strings of the character-creation screens (ShowCharacterInventory yendor2.asm:36150, ShowCharacterStats :36743, DrawQuitOrReturnLabel
:37632): Chapter 3 addresses: item pick headers 0x7D5A, name separator 0x7C92, stats headers 0x7D43, hotkey labels 0x8899, exit labels
0x7D17 / 0x7DB6. Dumps each.

    .
un_ida_script.ps1 dump_creation_item_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
# also the bottom-left exit label (DrawQuitOrReturnLabel): QUIT "CREATE" and RETURN
out = []
for addr, n in ((0x7D43, 24), (0x8899, 40), (0x7D17, 16), (0x7DB6, 8), (0x7D5A, 0x40), (0x7C92, 8)):
    data = bytes(ida_bytes.get_byte(DS_BASE + addr + i) for i in range(n))
    out.append((hex(addr), data))
text = repr(out)
open(r"C:\dev\yendor\yendor3\ida_scripts\creation_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

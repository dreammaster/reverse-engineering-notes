"""
ShowCharacterInventory (the character-creation item pick; yendor3.asm:36150) writes two header lines starting at DS:0x7A28 (Chapter 3
0x7D5A), a "NEXT" hotkey label at 0x7A3E (0x7D70) and DrawListEntryLabel joins an item's two name fields with the separator at 0x7C92
(0x7C92). Dumps them.

    .\run_ida_script.ps1 dump_creation_item_strings.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
# also the bottom-left exit label (DrawQuitOrReturnLabel): QUIT "CREATE" and RETURN
out = []
for addr, n in ((0x7D17, 16), (0x7DB6, 8), (0x7D5A, 0x40), (0x7C92, 8)):
    data = bytes(ida_bytes.get_byte(DS_BASE + addr + i) for i in range(n))
    out.append((hex(addr), data))
text = repr(out)
open(r"C:\dev\yendor\yendor3\ida_scripts\creation_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

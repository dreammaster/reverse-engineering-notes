"""
Strings of the character-creation screens (ShowCharacterInventory yendor2.asm:36150, ShowCharacterStats :36743, DrawQuitOrReturnLabel
:37632): the item pick headers at DS:0x7A28 and the name separator 0x7960, the stats screen headers 0x7A11 and its two hotkey labels
0x8572, the exit labels 0x79E5 / 0x7A84. Dumps each.

    .
un_ida_script.ps1 dump_creation_item_strings.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
# also the bottom-left exit label (DrawQuitOrReturnLabel): QUIT "CREATE" and RETURN
out = []
for addr, n in ((0x7A11, 24), (0x8572, 40), (0x79E5, 16), (0x7A84, 8), (0x7A28, 0x40), (0x7960, 8)):
    data = bytes(ida_bytes.get_byte(DS_BASE + addr + i) for i in range(n))
    out.append((hex(addr), data))
text = repr(out)
open(r"C:\dev\yendor\yendor2\ida_scripts\creation_item_strings.txt", "w", encoding="utf-8").write(text)
print(text)

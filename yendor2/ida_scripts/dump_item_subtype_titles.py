"""
Dump the remaining F5 item-subtype title strings to fully identify all
8 clue-book "INVENTORY ITEMS" subtypes. Already confirmed: subtype 7 =
TRANSPORTATIONS (0x8A6A), subtype 8 = WEAPONS (0x8A7A). This dumps
subtype 1 (0x8A01), subtype 3 (0x8A21), subtype 4 (0x8A3F), subtype 5
(0x8A54), subtype 6 (0x8A5C). Subtype 2 has no title (its loop is just
WaitForKeypress -- an empty/placeholder subtype).

Run via:
    .\\run_ida_script.ps1 dump_item_subtype_titles.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=40):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

labels = {1: 0x8A01, 3: 0x8A21, 4: 0x8A3F, 5: 0x8A54, 6: 0x8A5C}
for subtype, off in labels.items():
    ea = off + 0x2D860
    print(f"subtype {subtype}: {off:#x} -> {ea:#x}: {read_str(ea)!r}")

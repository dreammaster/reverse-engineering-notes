"""
Dumps the WORLD.DAT file offsets the item-data block-read stubs use
(PrepareItemDataBlockRead28/3A/22 read dwords at DS:0xCE43/0xCE47/0xCE4F,
with siblings at 0xCE4B/0xCE53).

    .\run_ida_script.ps1 dump_itemdata_offsets.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def dword(ea):
    return ida_bytes.get_word(ea) | (ida_bytes.get_word(ea + 2) << 16)


lines = []
for name, si in [("hdr28", 0xCE43), ("rec3A_a", 0xCE47), ("rec3A_b", 0xCE4B),
                  ("rec22_a", 0xCE4F), ("rec22_b", 0xCE53), ("next", 0xCE57)]:
    off = dword(DS_BASE + si)
    lines.append(f"{name}: DS:{si:#06x} -> {off:#x} ({off})")
out_path = r"C:\dev\yendor\yendor2\ida_scripts\itemdata_offsets.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

"""
Chapter 3 sibling of yendor2's dump_itemdata_offsets.py: the WORLD.DAT file
offsets of the item-data blocks (PrepareItemDataBlockRead28/3A/22 use DS:0xB1DB,
0xB1DF, 0xB1E7; their unnamed siblings 0xB1E3, 0xB1EB; the next at 0xB1EF).

    .\run_ida_script.ps1 dump_itemdata_offsets.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def dword(ea):
    return ida_bytes.get_word(ea) | (ida_bytes.get_word(ea + 2) << 16)


lines = []
for name, si in [("hdr28", 0xB1DB), ("rec3A_a", 0xB1DF), ("rec3A_b", 0xB1E3),
                  ("rec22_a", 0xB1E7), ("rec22_b", 0xB1EB), ("next", 0xB1EF)]:
    off = dword(DS_BASE + si)
    lines.append(f"{name}: DS:{si:#06x} -> {off:#x} ({off})")
out_path = r"C:\dev\yendor\yendor3\ida_scripts\itemdata_offsets.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

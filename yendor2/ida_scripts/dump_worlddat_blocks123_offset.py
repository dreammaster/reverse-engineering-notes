"""
Extends dump_worlddat_block4_offset.py to blocks 1/2/3, to find what
WORLD.DAT region immediately follows block4 (the spell/ability
catalog, 0x1AA5DD) so its real record count can be computed from a
byte-length difference, the same way block5/block6's sizes were
confirmed against monster.c's own MonsterCatalogLayout.

    .\run_ida_script.ps1 dump_worlddat_blocks123_offset.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def dword(ea):
    lo = ida_bytes.get_word(ea)
    hi = ida_bytes.get_word(ea + 2)
    return lo | (hi << 16)


lines = []
for name, si in [("block1", 0xCDFF),
                  ("block2", 0xCE03),
                  ("block3", 0xCDF3)]:
    ea = DS_BASE + si
    off = dword(ea)
    lines.append(f"{name}: DS:{si:#06x} (ea {ea:#x}) -> WORLD.DAT file offset {off:#x} ({off})")

out_path = r"C:\dev\yendor\yendor2\ida_scripts\worlddat_blocks123_offset.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))
print(f"wrote {out_path}")

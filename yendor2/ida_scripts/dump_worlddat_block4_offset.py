"""
Read-only: resolves the long-open "word_332D8-33306 encoded effect
descriptor cluster" write-site mystery for good. Confirmed by address
arithmetic that these fields all fall inside the 80-byte buffer
LoadClueBookSpellEntry fills (es:0x5A5A, es=word_2E4AA=seg seg129 --
i.e. linear seg129_base+0x5A5A, NOT a separate unwritten region at
all). LoadClueBookSpellEntry's own source is EMS page 0x5610, set up
by WorldDat_setBlock4 (yendor2.asm:43128): a WORLD.DAT block, record
size 0x50 (80) bytes, base file offset read from a small dword table
at DS:0xCE6B. This script reads that table (blocks 4/5/6, the 3
consecutive dwords at 0xCE63/0xCE67/0xCE6B) directly.

    .\run_ida_script.ps1 dump_worlddat_block4_offset.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def dword(ea):
    lo = ida_bytes.get_word(ea)
    hi = ida_bytes.get_word(ea + 2)
    return lo | (hi << 16)


lines = []
for name, si in [("block5 (WorldDat_setBlock5)", 0xCE63),
                  ("block6 (WorldDat_setBlock6)", 0xCE67),
                  ("block4 (WorldDat_setBlock4, LoadClueBookSpellEntry's own table)", 0xCE6B)]:
    ea = DS_BASE + si
    off = dword(ea)
    lines.append(f"{name}: DS:{si:#06x} (ea {ea:#x}) -> WORLD.DAT file offset {off:#x} ({off})")

out_path = r"C:\dev\yendor\yendor2\ida_scripts\worlddat_block4_offset.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))
print(f"wrote {out_path}")

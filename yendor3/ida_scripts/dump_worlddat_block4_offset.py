"""
Chapter 3 sibling of yendor2's dump_worlddat_block4_offset.py.
WorldDat_setBlock4 (yendor3.asm:43517) reads its own WORLD.DAT file
offset from DS:0xB203, record size 0x50 (80) bytes -- same shape as
Chapter 2. This is the "spell/ability record" table LoadClueBookSpellEntry
loads by index, which the long-open "word_332D8-33306 encoded effect
descriptor cluster" mystery turned out to just be named offsets INTO
(see file-formats.md's writeup).

    .\run_ida_script.ps1 dump_worlddat_block4_offset.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def dword(ea):
    lo = ida_bytes.get_word(ea)
    hi = ida_bytes.get_word(ea + 2)
    return lo | (hi << 16)


ea = DS_BASE + 0xB203
off = dword(ea)
line = f"block4 (WorldDat_setBlock4): DS:0xb203 (ea {ea:#x}) -> WORLD.DAT file offset {off:#x} ({off})"
print(line)
out_path = r"C:\dev\yendor\yendor3\ida_scripts\worlddat_block4_offset.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write(line)
print(f"wrote {out_path}")

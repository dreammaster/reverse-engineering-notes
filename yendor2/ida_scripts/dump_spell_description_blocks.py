"""
LookupSpellDescriptionBlockOffset (yendor2.asm, category 1-6): the WORLD.DAT blocks behind the clue book pages. 32-bit file offsets at
DS:0xD1A5 (stride 4) and 16-bit record sizes at DS:0xD1BD (stride 2); Chapter 3 0xB6FB / 0xB713. Dumps the six pairs.

    .\run_ida_script.ps1 dump_spell_description_blocks.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
OFFSETS = 0xD1A5
SIZES = 0xD1BD
lines = []
for i in range(6):
    off = ida_bytes.get_dword(DS_BASE + OFFSETS + 4 * i)
    size = ida_bytes.get_word(DS_BASE + SIZES + 2 * i)
    lines.append("category %d offset %d size %d" % (i + 1, off, size))
text = chr(10).join(lines)
open(r"C:\dev\yendor\yendor2\ida_scripts\spell_description_blocks.txt", "w", encoding="utf-8").write(text)
print(text)

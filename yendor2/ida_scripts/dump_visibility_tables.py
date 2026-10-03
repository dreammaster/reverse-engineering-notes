"""
ComputeDungeonCellVisibility (yendor2.asm:30448) walks occlusion tables at the very start of the data segment:
  DS:0x0E   (cell, list) word pairs ended by 0xFFFF: if the cell and the cell 8 bytes after it are both solid wall,
            every cell in the FFFF-terminated list at `list` is hidden
  DS:0xE0   one list pointer per viewport cell 0x11..0x32 (index (depth - 0x11) * 2), 0 = none
The cells and lists are offsets into the 6D60 viewport buffer. Dumps the first 0x800 bytes as words.

    .\run_ida_script.ps1 dump_visibility_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
words = [ida_bytes.get_word(DS_BASE + 2 * i) for i in range(0x400)]
lines = [" ".join(f"{w:04X}" for w in words[i:i + 16]) for i in range(0, len(words), 16)]
with open(r"C:\dev\yendor\yendor2\ida_scripts\visibility_tables.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n".join(lines))

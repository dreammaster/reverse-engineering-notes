"""
Chapter 3 sibling: the 5000-byte new-game template's WORLD.DAT offset is the dword at DS:0xB207.

    .\run_ida_script.ps1 dump_newgame_block.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
lo = ida_bytes.get_word(DS_BASE + 0xB207)
hi = ida_bytes.get_word(DS_BASE + 0xB207 + 2)
line = f"newgame template offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), size 5000"
out_path = r"C:\dev\yendor\yendor3\ida_scripts\newgame_block.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write(line)
print(line)

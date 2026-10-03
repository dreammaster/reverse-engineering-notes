"""
PreloadMonsterStatsTable (misnamed) reads one 0x499C-byte block of WORLD.DAT into word_3291A's segment: the first-person
view's geometry tables (_val11.._val14, _ptr1.._ptr7 index into it: DrawViewportSprite). The file offset is the dword at
DS:0xCE5B (Chapter 3 DS:0xB1F3, found via seg133).

    .\run_ida_script.ps1 dump_view_tables_offset.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lo = ida_bytes.get_word(DS_BASE + 0xCE5B)
hi = ida_bytes.get_word(DS_BASE + 0xCE5B + 2)
off = (hi << 16) | lo
text = f"view tables offset = {off} (0x{off:X}), size 0x499C"
open(r"C:\dev\yendor\yendor2\ida_scripts\view_tables_offset.txt", "w", encoding="utf-8").write(text)
print(text)

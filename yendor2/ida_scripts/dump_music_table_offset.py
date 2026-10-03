"""
UpdateAmbientMusicForRegion (yendor2.asm:37699) reads a 2-byte record (a music track id) per map page from WORLD.DAT;
PrepareAmbientMusicBlockRead (:43224) takes the 32-bit file offset from the dword at DS:0xCDFB.

    .\run_ida_script.ps1 dump_music_table_offset.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lo = ida_bytes.get_word(DS_BASE + 0xCDFB)
hi = ida_bytes.get_word(DS_BASE + 0xCDFB + 2)
line = f"ambient music table offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), record size 2"
with open(r"C:\dev\yendor\yendor2\ida_scripts\music_table_offset.txt", "w", encoding="utf-8") as f:
    f.write(line)
print(line)

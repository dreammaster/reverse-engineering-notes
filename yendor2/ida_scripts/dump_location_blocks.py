"""
The two WORLD.DAT blocks behind the clue book's map locations (BuildClueLocationSuffix yendor2.asm:31620): WorldDat_setBlock3 (6-byte records,
file offset at DS:0xCDF3) and PrepareClueLocationSuffixBlockRead (20-byte records, offset at DS:0xCDF7). Chapter 3: 0xB187 / 0xB18B. Dumps the
two 32-bit offsets.

    .\run_ida_script.ps1 dump_location_blocks.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
text = "block3 %d\nblock4 %d" % (ida_bytes.get_dword(DS_BASE + 0xCDF3), ida_bytes.get_dword(DS_BASE + 0xCDF7))
open(r"C:\dev\yendor\yendor2\ida_scripts\location_blocks.txt", "w", encoding="utf-8").write(text)
print(text)

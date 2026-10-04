"""
The two WORLD.DAT blocks behind the clue book's map locations (BuildClueLocationSuffix yendor3.asm:31620): WorldDat_setBlock3 (6-byte records,
file offset at DS:0xB187) and PrepareClueLocationSuffixBlockRead (20-byte records, offset at DS:0xB18B). Chapter 2: 0xCDF3 / 0xCDF7. Dumps the
two 32-bit offsets.

    .\run_ida_script.ps1 dump_location_blocks.py -NoExport
"""
import ida_bytes

import ida_segment
DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
text = "block3 %d\nblock4 %d" % (ida_bytes.get_dword(DS_BASE + 0xB187), ida_bytes.get_dword(DS_BASE + 0xB18B))
open(r"C:\dev\yendor\yendor3\ida_scripts\location_blocks.txt", "w", encoding="utf-8").write(text)
print(text)

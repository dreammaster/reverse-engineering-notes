"""
WorldDat_setBlock3 (yendor2.asm:43109) locates the per-page location table: 6-byte records
indexed by map page ((y / 24) * 20 + x / 40), read by BuildClueLocationSuffix and
ReadMapCellAttributeByte. The 32-bit WORLD.DAT offset is the dword at DS:0xCDF3.

    .\run_ida_script.ps1 dump_pagetable_offset.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lo = ida_bytes.get_word(DS_BASE + 0xCDF3)
hi = ida_bytes.get_word(DS_BASE + 0xCDF3 + 2)
line = f"page table offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), record size 6"
with open(r"C:\dev\yendor\yendor2\ida_scripts\pagetable_offset.txt", "w", encoding="utf-8") as f:
    f.write(line)
print(line)

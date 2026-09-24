"""
Read-only: dumps the 89 x 4-byte packed-BCD XP-threshold table
CheckForLevelUp (yendor3.asm:12025) walks at DS:0xC75F -- Chapter 3's
equivalent of yendor2's 0x9277 table (see that game's own dump script
for the derivation of the 89-entry count from the loop's own bound).

    .\run_ida_script.ps1 dump_xp_threshold_table.py -NoExport
"""
import idc
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
TABLE_OFFSET = 0xC75F
ENTRY_COUNT = 89
ENTRY_SIZE = 4

ea0 = DS_BASE + TABLE_OFFSET
print(f"table at DS:{TABLE_OFFSET:#06x} (ea {ea0:#x}), {ENTRY_COUNT} entries x {ENTRY_SIZE} bytes")

for i in range(ENTRY_COUNT):
    ea = ea0 + i * ENTRY_SIZE
    raw = [idc.get_wide_byte(ea + j) for j in range(ENTRY_SIZE)]
    hexstr = " ".join(f"{b:02x}" for b in raw)
    print(f"level {i+1:3d}: {hexstr}")

"""
Read-only: dumps the 65 x 4-byte packed-BCD XP-threshold table
CheckForLevelUp (yendor2.asm:20036) walks at DS:0x9277 -- one entry per
character level, compared against a character's own Bcd4 experience
field via CompareBCD4.

    .\run_ida_script.ps1 dump_xp_threshold_table.py -NoExport
"""
import idc

DS_BASE = 0x2D860
TABLE_OFFSET = 0x9277
ENTRY_COUNT = 89  # (0x93DB - 0x9277) / 4, the loop's own upper bound -- not 65, an earlier guess
ENTRY_SIZE = 4

ea0 = DS_BASE + TABLE_OFFSET
print(f"table at DS:{TABLE_OFFSET:#06x} (ea {ea0:#x}), {ENTRY_COUNT} entries x {ENTRY_SIZE} bytes")

for i in range(ENTRY_COUNT):
    ea = ea0 + i * ENTRY_SIZE
    raw = [idc.get_wide_byte(ea + j) for j in range(ENTRY_SIZE)]
    hexstr = " ".join(f"{b:02x}" for b in raw)
    print(f"level {i+1:3d}: {hexstr}")

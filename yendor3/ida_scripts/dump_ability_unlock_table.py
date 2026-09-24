"""
Read-only: dumps the class/level-indexed ability-unlock table
UseTrainingItem walks at DS:0xB8B5 -- Chapter 3's equivalent of
yendor2's DS:0xD22B (see that game's own dump script). Confirmed exactly
6 rows: 0xB8B5 + 6*0x50 == 0xBA95, TravelToDestination's own destination
table base in this game -- reading further would walk into that
unrelated table, exactly as yendor2's own boundary check confirmed.

    .\run_ida_script.ps1 dump_ability_unlock_table.py -NoExport
"""
import idc
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF
TABLE_OFFSET = 0xB8B5
ROW_COUNT = 6
ROW_SIZE = 0x50
COL_COUNT = 20
COL_SIZE = 4

ea0 = DS_BASE + TABLE_OFFSET
print(f"table at DS:{TABLE_OFFSET:#06x} (ea {ea0:#x}), {ROW_COUNT} rows x {ROW_SIZE:#x} bytes")

for row in range(ROW_COUNT):
    rowEa = ea0 + row * ROW_SIZE
    entries = []
    for col in range(COL_COUNT):
        colEa = rowEa + col * COL_SIZE
        a = idc.get_wide_word(colEa)
        b = idc.get_wide_word(colEa + 2)
        entries.append(f"{a:04x}/{b:04x}")
    print(f"row {row}: " + " ".join(entries))

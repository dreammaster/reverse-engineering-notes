"""
Read-only: dumps the class/level-indexed ability-unlock table
UseTrainingItem (yendor2.asm:21764 on) walks at DS:0xD22B -- 10 rows
(one per class-base-and-tier bucket, stride 0x50=80 bytes) x 20 columns
(one per even level 2..40, stride 4 bytes) x 2 u16 ability-flag ids
(0 = no entry).

    .\run_ida_script.ps1 dump_ability_unlock_table.py -NoExport
"""
import idc

DS_BASE = 0x2D860
TABLE_OFFSET = 0xD22B
ROW_COUNT = 10
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

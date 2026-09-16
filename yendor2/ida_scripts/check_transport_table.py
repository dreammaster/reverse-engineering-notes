"""
Read-only: dumps the two tables sub_1CDBC scans (0x9519, 6 entries of
4 bytes; 0x95EB, 4 entries of 2 bytes -- the latter is the SAME address
as word_36E4B, which RunTitleScreen sums to decide whether to set a
flag, tying these together) and checks their current values.

    .\run_ida_script.ps1 check_transport_table.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

print("table @ 0x9519 (6 x 4-byte entries):")
ea = DS_BASE + 0x9519
for i in range(6):
    v = ida_bytes.get_word(ea + i * 4)
    v2 = ida_bytes.get_word(ea + i * 4 + 2)
    print(f"  [{i}] {v:#x} {v2:#x}")

print("\ntable @ 0x95EB (4 x 2-byte entries, == word_36E4B):")
ea = DS_BASE + 0x95EB
for i in range(4):
    v = ida_bytes.get_word(ea + i * 2)
    print(f"  [{i}] {v:#x}")

import idc
print("\nname at 0x95EB:", idc.get_name(DS_BASE + 0x95EB))
print("name at 0x9519:", idc.get_name(DS_BASE + 0x9519))

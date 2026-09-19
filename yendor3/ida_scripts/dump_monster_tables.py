"""
Read-only: Chapter 3's counterpart of yendor2/ida_scripts/dump_monster_tables.py.
Offsets of the monster stat blocks (DS:0xB1FB) and type-to-block lookup
(DS:0xB1FF) with their neighbours, and the in-EXE death-flag table at
DS:0xCE51. This database has no ds segment register, so the data segment
base is seg133's paragraph-aligned start.

    .\run_ida_script.ps1 dump_monster_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

print("OFFSETS")
for off in range(0xB1F7, 0xB207, 4):
    print("  DS:%#x = %#x" % (off, ida_bytes.get_wide_dword(DS_BASE + off)))

print("DEATH FLAG TABLE @0xCE51 (type id, flagA, flagB), signed")
ea = DS_BASE + 0xCE51
rows = []
while True:
    type_id, a, b = [ida_bytes.get_wide_word(ea + 2 * k) for k in range(3)]
    if type_id == 0:
        break
    rows.append((type_id, a - 65536 if a >= 32768 else a, b - 65536 if b >= 32768 else b))
    ea += 6
print("  count %d" % len(rows))
print("  rows %s" % (rows,))

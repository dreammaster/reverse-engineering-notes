"""
Read-only: dumps what src23/monster.c needs from Chapter 2's database.

  1. The WORLD.DAT base offsets of the monster stat blocks (DS:0xCE63,
     WorldDat_setBlock5, 106-byte blocks) and the type-to-block lookup
     (DS:0xCE67, WorldDat_setBlock6, u16 entries), plus the offsets that
     follow them so the table sizes can be derived from the differences.
  2. The in-EXE table at DS:0xE4E9 that SpawnMonsterInFacingDirection
     searches: 6-byte entries (type id, flag, flag), ascending, ended by a
     zero id. A monster's death sets (>0) or clears (<0) the global flags.

    .\run_ida_script.ps1 dump_monster_tables.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860

print("OFFSETS")
for off in (0xCE5F, 0xCE63, 0xCE67, 0xCE6B):
    print("  DS:%#x = %#x" % (off, ida_bytes.get_wide_dword(DS_BASE + off)))

print("DEATH FLAG TABLE @0xE4E9 (type id, flagA, flagB), signed")
ea = DS_BASE + 0xE4E9
rows = []
while True:
    type_id, a, b = [ida_bytes.get_wide_word(ea + 2 * k) for k in range(3)]
    if type_id == 0:
        break
    rows.append((type_id, a - 65536 if a >= 32768 else a, b - 65536 if b >= 32768 else b))
    ea += 6
print("  count %d" % len(rows))
print("  rows %s" % (rows,))

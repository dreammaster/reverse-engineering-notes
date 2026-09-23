"""
Read-only, yendor3 port: dumps the 4 facing-dependent spawn-position
offset tables SpawnMonsterInFacingDirection indexes, to check whether
they're identical to yendor2's.

    .\run_ida_script.ps1 dump_spawn_offset_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

tables = {
    "North": 0x73BE,
    "South": 0x7424,
    "East": 0x748A,
    "West": 0x74F0,
}

ENTRY_COUNT = 51

for name, off in tables.items():
    ea = DS_BASE + off
    entries = []
    for i in range(ENTRY_COUNT):
        dx = ida_bytes.get_wide_byte(ea + i * 2)
        dy = ida_bytes.get_wide_byte(ea + i * 2 + 1)
        if dx >= 0x80:
            dx -= 0x100
        if dy >= 0x80:
            dy -= 0x100
        entries.append((dx, dy))
    print(f"{name} ({off:#x}, ea={ea:#x}): {entries}")

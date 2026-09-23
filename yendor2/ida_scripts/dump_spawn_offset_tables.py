"""
Read-only: dumps the 4 facing-dependent spawn-position-offset tables
SpawnMonsterInFacingDirection indexes by g_viewportRowDepth*2 (North
0x7090, South 0x70F6, East 0x715C, West 0x71C2 -- each entry is a
signed-byte (deltaX, deltaY) pair added to the party's world position).

    .\run_ida_script.ps1 dump_spawn_offset_tables.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg129").start_ea & ~0xF

tables = {
    "North": 0x7090,
    "South": 0x70F6,
    "East": 0x715C,
    "West": 0x71C2,
}

ENTRY_COUNT = 51  # (0x70F6 - 0x7090) / 2

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

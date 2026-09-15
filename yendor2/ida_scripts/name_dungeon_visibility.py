"""
Names sub_213FC, called (before RenderDungeonViewport's three render
passes) right after BuildDungeonViewportCells in RedrawDungeonScreen.
Computes line-of-sight occlusion: marks cells that should be hidden
(behind a wall corner, etc.) with the same [+6] bit 0 flag every
render-pass function checks (DrawDungeonCellWallTexture,
ExtendDungeonFloorTexture, ExtendDungeonCeilingTexture, etc. all skip
a cell when this bit is set) -- this is that flag's origin.

Mechanism: walks progressively closer rows (0x2D/0x2A/0x27/0x22/0x11,
via sub_214F4, a per-row "is this row wall-blocked" check) to find the
nearest row not blocked by a wall type in range [_val18.._val17], then
for that row and every closer one, marks the [+6] bit-0 "hidden" flag
on every cell reachable from a side-passage index list (di+2 pointing
to a -1-terminated array of cell pointers) -- effectively "these side
cells are behind a wall and shouldn't be drawn". Then a second phase
walks a separate 0x6EF0 buffer (33 entries) doing the same lookup
against a direction-indexed table at 0xE0.

-> ComputeDungeonCellVisibility

Run via:
    .\run_ida_script.ps1 name_dungeon_visibility.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x213FC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeDungeonCellVisibility", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeDungeonCellVisibility': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes line-of-sight occlusion for the dungeon viewport: marks "
    "cells that should be hidden (e.g. behind a wall corner) with the "
    "[+6] bit 0 'hidden' flag every render-pass function this session "
    "checks (DrawDungeonCellWallTexture, ExtendDungeonFloorTexture, "
    "ExtendDungeonCeilingTexture, etc.) -- this is that flag's origin. "
    "Walks progressively closer rows via sub_214F4 to find the nearest "
    "wall-blocked boundary, then marks side-passage cells hidden past "
    "it. Called from RedrawDungeonScreen after BuildDungeonViewportCells.",
    False,
)

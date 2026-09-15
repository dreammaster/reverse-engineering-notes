"""
Names sub_21306 and sub_213D4 -- the setup step that builds the local
0x6D60 scratch cell buffer every dungeon-rendering pass
(RenderDungeonViewRow, ExtendDungeonFloorTexture,
ExtendDungeonCeilingTexture, etc.) actually reads from.

sub_21306 (-> BuildDungeonViewportCells): called first thing in
RedrawDungeonScreen. Computes a facing-dependent row stride (bp =
word_2E55E) and side-step (bx/cx offsets), selected by the same
word_36CF5 facing-tier bits used throughout (ShowCompassDirection,
SpawnMonsterInFacingDirection, etc.), then a source pointer (es:si)
into the level's map data from the current position
(word_36CF7/36CF9) plus that offset. Calls sub_213D4 7 times (the
same 0x11/0x11/5/3/3/3(/implicit 7th) row-count pattern as
RenderDungeonViewport) to copy each visible row into the scratch
buffer at di=0x6D60, advancing the destination by a fixed per-row
amount (`add si,bp` six times between calls) each time.

sub_213D4 (-> CopyDungeonRowCells): the row-copy primitive -- copies
one 8-byte cell record at a time (es:[si] -> [di]) for cx cells,
advancing si by bp (the facing-dependent stride) and di by 8, then a
final si += word_2E560 (a side-step to the next row's start).

Run via:
    .\run_ida_script.ps1 name_build_viewport_cells.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x21306: "BuildDungeonViewportCells",
    0x213D4: "CopyDungeonRowCells",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x21306,
    "Builds the local scratch cell buffer (di=0x6D60) that every "
    "dungeon-rendering pass reads from: computes a facing-dependent "
    "row stride/side-step (word_36CF5 tier bits) from the current "
    "position, then calls CopyDungeonRowCells 7x (same row-count "
    "pattern as RenderDungeonViewport) to copy the visible cells from "
    "the level's map data. Called first in RedrawDungeonScreen.",
    False,
)
ida_bytes.set_cmt(
    0x213D4,
    "Copies one row of 8-byte cell records from the level's map data "
    "(es:si, advancing by bp, the facing-dependent stride) into the "
    "scratch viewport buffer (di, advancing by 8), then steps si to "
    "the next row's start (+= word_2E560). Called 7x by "
    "BuildDungeonViewportCells.",
    False,
)

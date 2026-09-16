"""
Names two small standalone visual-effect helpers on the offscreen
video buffer (_videoBufferSeg).

sub_2BC56, called once from the large unnamed combat dispatcher
sub_2C0FE: darkens a rectangular region by subtracting 2 from every
byte (palette index) in it. 136 rows x 224 cols, starting at offset
0xA08 (row 8, col 8 in the 320-byte-wide mode-13h buffer -- matching
the dungeon viewport's on-screen position/size), row stride 320
(224 touched + 96 skipped) -- i.e. one step of a palette-index-shift
darkening effect over the dungeon viewport, plausibly a hit-flash or
transition dim. -> DimDungeonViewport

sub_2BBD7, called from the already-named HandleRangedOrCombatAction
and HighlightSelectedAbilityIcon: fills a region of the offscreen
buffer with byte 0xFF via 0x69 (105) row-strided `rep stosw` passes
(210 bytes touched per row, but only 110-byte stride between rows, so
consecutive passes overlap/extend a single ~11.7KB contiguous block
starting at offset 0) -- clearing/resetting whatever mask or overlay
buffer backs the action-icon highlight effect before it's redrawn.
-> ClearActionIconHighlightMask

Run via:
    .\run_ida_script.ps1 name_dim_viewport_and_clear_mask.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2BC56: "DimDungeonViewport",
    0x2BBD7: "ClearActionIconHighlightMask",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2BC56,
    "Darkens a 224x136 region of the offscreen buffer (starting at "
    "row 8, col 8 of the 320-wide mode-13h buffer -- the dungeon "
    "viewport's area) by subtracting 2 from every byte (palette "
    "index) -- one step of a fade/dim effect. Called from sub_2C0FE.",
    False,
)
ida_bytes.set_cmt(
    0x2BBD7,
    "Fills ~11.7KB of the offscreen buffer (starting at offset 0) "
    "with byte 0xFF via 105 overlapping row-strided rep stosw passes "
    "-- clears whatever mask/overlay buffer backs the action-icon "
    "highlight effect. Called from HandleRangedOrCombatAction and "
    "HighlightSelectedAbilityIcon.",
    False,
)

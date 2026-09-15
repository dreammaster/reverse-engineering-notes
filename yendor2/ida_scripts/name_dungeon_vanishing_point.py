"""
Names sub_21217, RenderDungeonViewport's 7th and final call (word_328F2
row pointer, cx=3, but structurally different from
RenderDungeonViewRow) -- draws the vanishing-point / far-wall cells at
the end of the visible corridor.

For two adjacent cell positions (di and di+0x10 -- left and right far
corners), draws a far-wall/ceiling picture (the 0xE551 table's [+6]
field, distinct from [+4] used for regular walls) at z-layer 6 with an
extra offset (word_32932) for the second position, then for the final
di position: calls DrawDungeonCellSideFeature if a side-feature is
present, calls TryTriggerMonsterEncounterAtCell (same per-cell
encounter check as RenderDungeonViewRow), and conditionally calls
sub_212EB (gated on word_328CA bit 0x1000 -- plausibly a
darkness/torch-needed overlay, not traced).

-> RenderDungeonVanishingPoint

Run via:
    .\run_ida_script.ps1 name_dungeon_vanishing_point.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21217
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RenderDungeonVanishingPoint", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RenderDungeonVanishingPoint': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws the far-wall/vanishing-point cells at the end of the "
    "visible corridor (0xE551 table's [+6] field, z-layer 6, two "
    "adjacent positions), then DrawDungeonCellSideFeature + "
    "TryTriggerMonsterEncounterAtCell for the final cell, plus a "
    "conditional sub_212EB (word_328CA bit 0x1000, not traced). "
    "Called once by RenderDungeonViewport as its 7th/final row.",
    False,
)

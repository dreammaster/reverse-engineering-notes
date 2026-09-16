"""
Corrects RenderDungeonViewport's pre-existing comment. It previously
called word_328E6..word_328F2 "different row-data pointers" -- wrong.
Tracing ShiftPaletteShadeClamped's callers this round showed
word_32926 (which RenderDungeonViewport copies each of these into,
one per RenderDungeonViewRow call) is the shade-shift delta
DrawPicture/ShiftPaletteShadeClamped apply per pixel. The SAME 7
globals are also walked by ApplyDistanceShadingToFloorOrCeiling (was
sub_29FF6) as its own per-row-band shade deltas for the floor/ceiling.
So word_328E6-word_328F2 is a shared 7-entry per-depth-row shade-delta
gradient table, applied consistently to walls/monsters (via
word_32926) and floor/ceiling (via ApplyDistanceShadingToFloorOrCeiling)
for the same 7 depth bands -- not pointers of any kind.

Run via:
    .\run_ida_script.ps1 fix_render_dungeon_viewport_comment.py
"""
import ida_bytes

ea = 0x21594  # address of "mov di, 6D60h" -- see below, resolved by name
# Resolve the exact address from the function name instead of a guess.
import idc
ea = idc.get_name_ea_simple("RenderDungeonViewport")
ida_bytes.set_cmt(
    ea,
    "First-person dungeon corridor viewport renderer: resets "
    "word_3292C, calls RenderDungeonViewRow 7x with decreasing cell "
    "counts (0x11/0x11/5/3/3/3/3), copying each of word_328E6.."
    "word_328F2 into word_32926 first -- the shared per-depth-row "
    "shade-delta gradient (same table ApplyDistanceShadingToFloor"
    "OrCeiling walks for the floor/ceiling) that "
    "DrawPicture/ShiftPaletteShadeClamped apply per pixel, giving "
    "walls/monsters and floor/ceiling consistent distance-based "
    "lighting falloff. Called from sub_20C1E and sub_20C46.",
    False,
)
print(f"Comment updated for RenderDungeonViewport at {ea:#x}")

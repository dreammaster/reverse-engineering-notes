"""
Names sub_29FF6, called twice from DrawDungeonFloorAndCeiling -- an
important find: applies a graduated distance-shading gradient to the
floor or ceiling texture already drawn in the viewport buffer.

Branches on word_2E532 (the "scale class" parameter from
DrawViewportSprite's shared convention) == 0x40: each branch starts
CopyShadedViewportRows with SI and DI set to the *same* address
(0xA08 for one branch, 0x5788 for the other) -- so it isn't copying
from elsewhere, it's re-shading the buffer in place, band by band.

Each branch walks a fixed 7-entry gradient table (word_328E6,
word_328E8, word_328EA, word_328EC, word_328EE, word_328F0,
word_328F2 -- 7 consecutive words, precomputed elsewhere, not traced
this round) as successive per-row-band shade deltas, applying each to
a different-sized run of rows (10/11/10/10/9/9/3 rows for the 0xA08
branch, totaling 62; 4/9/9/10/10/11/21 for the 0x5788 branch in
reverse gradient order, totaling 74) via CopyShadedViewportRows. This
is very likely the mechanism behind the documented "plausible torch-
fuel/light-source" derived stat (+0x64): a precomputed 7-step
brightness falloff applied per row-band with distance from the
viewer, separately for the floor (one address/branch) and ceiling
(the other).

-> ApplyDistanceShadingToFloorOrCeiling

Run via:
    .\run_ida_script.ps1 name_apply_floor_ceiling_shading.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29FF6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyDistanceShadingToFloorOrCeiling", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyDistanceShadingToFloorOrCeiling': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Re-shades the floor or ceiling viewport buffer in place "
    "(CopyShadedViewportRows with si==di), branching on word_2E532 "
    "for floor (0xA08) vs ceiling (0x5788) addressing, walking a "
    "fixed 7-entry shade-delta gradient table (word_328E6..word_328F2) "
    "across successive row-bands -- a distance/light falloff effect, "
    "plausibly tied to the torch-fuel-like derived stat +0x64. Called "
    "twice from DrawDungeonFloorAndCeiling.",
    False,
)

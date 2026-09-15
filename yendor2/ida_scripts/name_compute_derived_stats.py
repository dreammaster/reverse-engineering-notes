"""
Names sub_23F58, called from ShowCharacterSkills and ShowCharacterStats
right after RollCharacterAttributes: computes a whole family of
derived character stats from the 6 base attributes
(+0x3C/+0x3E/+0x40/...), each a weighted percentage blend
(ScaleByPercentRounded) of 2-3 attributes plus a small class-based
bonus/penalty (selected by [+0xE], the class id), mirrored into a
"+0x40 higher" derived-field pair matching RollCharacterAttributes'
own base/derived convention: +0x58/+0x98, +0x5A/+0x9A, +0x5C/+0x9C,
+0x5E/+0x9E, +0x60/+0xA0, and more.

This confirms +0x58 (already documented as "plausibly a perception/
identify stat gating DrawMonsterInfoPanel's detail reveal") is a
*derived* stat -- a blend of the first 3 attributes plus a
class-dependent bonus -- not a raw rolled value. The remaining
derived fields (+0x5A onward) aren't individually identified yet.

-> ComputeDerivedCharacterStats

Run via:
    .\run_ida_script.ps1 name_compute_derived_stats.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23F58
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeDerivedCharacterStats", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeDerivedCharacterStats': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes a family of derived stats from the 6 base attributes: "
    "each a weighted percentage blend (ScaleByPercentRounded) of 2-3 "
    "attributes plus a class-dependent bonus, mirrored into current/"
    "max pairs +0x58/+0x98, +0x5A/+0x9A, +0x5C/+0x9C, +0x5E/+0x9E, "
    "+0x60/+0xA0, and more. Confirms +0x58 (DrawMonsterInfoPanel's "
    "reveal-gate stat) is derived, not raw-rolled. Called from "
    "ShowCharacterSkills and ShowCharacterStats.",
    False,
)

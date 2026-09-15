"""
Names sub_1D9E5, called from sub_1DA60 (twice) and sub_1DA2C: applies
a resolved attack's damage to the target (si), with elemental
resistance.

If word_2E49C (the damage amount) is nonzero: ORs display/wound flags
into [si+0xC] (word_2E49A masked by ~[si+0x96], a per-target
display-suppression mask), then reduces the damage via a bit-scan
loop comparing the attack's damage-type flags (word_2E49E) against
the target's resistance flags ([si+0x98]) -- each set resistance bit
matching an attack-type bit halves the damage (ax=word_2E49C shifted
right once per match, cx=0x10 max iterations). Subtracts the reduced
damage from HP ([si+0x10], clamped to 0), sets display flags
([si+0xC] |= 3), and sets word_328C8 bit 0x200 (a "damage applied"
flag).

This is the shared damage-application step behind
ApplyDamageToMapMonster's word_2E49A/49C mechanism -- confirms
[si+0x96]/[si+0x98] are per-target resistance/display-suppression
fields (part of the same resistance-flag cluster already flagged as
"individual fields not yet matched to specific resistance types" in
the ability-cost documentation).

-> ApplyResolvedDamageWithResistance

Run via:
    .\run_ida_script.ps1 name_apply_resolved_damage.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D9E5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyResolvedDamageWithResistance", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyResolvedDamageWithResistance': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Applies a resolved attack's damage (word_2E49C) to the target "
    "(si), reducing it via a resistance bit-scan (word_2E49E attack "
    "type flags vs [si+0x98] resistance flags -- each match halves "
    "the damage), then subtracts from HP ([si+0x10], clamped to 0) "
    "and sets display flags. Shared by ranged/ability attacks "
    "(sub_1DA60) and sub_1DA2C.",
    False,
)

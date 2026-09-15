"""
Names sub_1DB73, called from sub_1DA60's non-ranged-weapon branch: an
ability/spell effect resolver against a target (si).

Rolls an 85% success chance (RandomInRange(100) <= 0x55), then
dispatches on word_32974 (the ability/spell id -- the same selector
CastSpell/RestCharacter/etc. use) to set a flat damage amount
(word_2E49C) and, for several ids, a status-effect flag (word_2E49A,
matching ApplyStatusEffect's bit convention) plus a duration
([si+0x1C]/[si+0x1E], matching TickStatusEffects' counters) -- unless
the target already has that status ([si+0x96] bit test), in which case
only the damage is set. Two specific ids (_val46/_val29) are gated on
NOT being in combat (word_328CA bit 0x1000) and, per sub_1DA60's
caller logic, are the two ids that trigger the multi-row
ApplyDamageAlongCorridorLine area effect -- matching the "IN A
STRAIGHT LINE"/"IN A 3X3 AREA" targeting text from
ShowClueBookSpellDetail's message dump.

-> ResolveAbilityEffect

Run via:
    .\run_ida_script.ps1 name_resolve_ability_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1DB73
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResolveAbilityEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResolveAbilityEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Ability/spell effect resolver: 85% success roll, then dispatches "
    "on word_32974 (ability id) to set a flat damage amount "
    "(word_2E49C) and, for several ids, a status-effect flag "
    "(word_2E49A) plus duration ([si+0x1C]/[0x1E]) unless already "
    "afflicted ([si+0x96]). Two ids (area-effect spells, per "
    "ShowClueBookSpellDetail's targeting text) are gated on not being "
    "in combat. Called from sub_1DA60.",
    False,
)

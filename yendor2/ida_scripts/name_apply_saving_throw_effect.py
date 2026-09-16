"""
Names sub_28BD2, called from UseAbilityCommand and sub_2A788: applies
a trap/ability effect gated by a saving throw.

If word_32DD0 is 0, no-ops. Otherwise calls FailsSavingThrow(threshold=
word_32DC0, resistance bonus=[current char]+0x6C -- the same field
already documented as plausibly a lockpicking/perception skill via
ShowLockStatus; using it as a saving-throw resistance bonus here fits a
general perception/awareness stat better than lockpicking specifically).
If the target passes the save (FailsSavingThrow returns 0), nothing
happens. If it fails: an effect id (word_32DC2) < 50 is a
*single-target* effect applied to the current character only (matched
into its own icon-bar slot via g_partySlotAssignment); an id >= 50 is
"single-id - 50" applied to *every* non-incapacitated party member (the
already-familiar +0x1C bits 0x1C40 skip check) -- i.e. effect ids 50+
are the party-wide/area variant of the id 50 lower. Either way,
populates the matching icon-bar slot(s) via PrepareTrapEffectSlots and
finishes with ApplyEffectAndDrawIconBar. -> ApplySavingThrowEffect

Run via:
    .\run_ida_script.ps1 name_apply_saving_throw_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28BD2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplySavingThrowEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplySavingThrowEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gated by FailsSavingThrow (threshold word_32DC0, resistance bonus "
    "= current character's +0x6C). On a failed save: effect id "
    "word_32DC2 < 50 applies to the current character only; id >= 50 "
    "applies (id-50) to every non-incapacitated party member -- ids 50+ "
    "are the party-wide variant of the id 50 lower. Populates the "
    "matching icon-bar slot(s) via PrepareTrapEffectSlots and finishes "
    "with ApplyEffectAndDrawIconBar. Called from UseAbilityCommand and "
    "sub_2A788.",
    False,
)

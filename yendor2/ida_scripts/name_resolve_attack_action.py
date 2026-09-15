"""
Names sub_1DA60, called from sub_1D4B8 (the combat-round driver, not
yet fully named) and its own chunk. Resolves an attack/ability action
against the current target (word_328D4).

If word_328C8 bit 0x100 is set (a "ranged/thrown weapon" mode the
caller sets before calling, e.g. start's loc_10A65 branch): finds an
equipped item in one of the 4 party inventory slots (0x5078/
g_partySlotAssignment), loads its catalog record, sets damage-type
flags (word_2E49E) from the item's own flags, then rolls the hit via
ResolveAttack (attacker's [si+0x58] stat vs. the target's [+0x48]/
[+0x4A] accuracy/defense) and applies it via
ApplyResolvedDamageWithResistance.

Otherwise: calls ResolveAbilityEffect (a spell/ability roll keyed on
word_32974). If it resolved to one of the two area-effect ability ids,
and the party isn't already in formal combat, picks one of 4 depth-row
triples (based on which band the current word_3292C falls in) and
calls ApplyDamageAlongCorridorLine for each -- a "probe nearby depth
bands for a target" step for an area spell cast before combat has
formally started.

-> ResolveAttackOrAbilityAction

Run via:
    .\run_ida_script.ps1 name_resolve_attack_action.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1DA60
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResolveAttackOrAbilityAction", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResolveAttackOrAbilityAction': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves an attack/ability action against word_328D4 (current "
    "target). word_328C8 bit 0x100 set -> ranged/thrown weapon attack "
    "(finds an equipped item, ResolveAttack + "
    "ApplyResolvedDamageWithResistance). Else -> ResolveAbilityEffect "
    "(spell/ability roll); for its 2 area-effect ids, when not yet in "
    "formal combat, probes nearby depth-row triples via "
    "ApplyDamageAlongCorridorLine to find a target. Called from "
    "sub_1D4B8 (the combat-round driver).",
    False,
)

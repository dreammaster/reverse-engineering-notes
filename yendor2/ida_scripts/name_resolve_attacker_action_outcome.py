"""
Names sub_16BF6, called twice from sub_16881 (the combat dispatcher)
with si=word_32904 (attacker record) and di=word_32908 (defender
record) already set up.

Three outcome branches, selected by word_328CA bit 0x200 and the
attacker's [+0x92] flag bits 0xE00 (special-attack-type flags):

1. Normal path (bit 0x200 clear, or attacker's 0xE00 flags clear, or
   an attacker BCD4 resource at [+0x8E] below a threshold): calls the
   confirmed ResolveAttack(defense, accuracy, weaponPower) and, on
   nonzero damage, records the hit into an icon-bar-style structure
   at word_32906 for later presentation.

2. Status-effect path (word_328CA bit 0x200 set, attacker flags set,
   resource check passed): a FailsSavingThrow roll using the
   defender's own stats; on a failed save, records a status-effect
   application instead of direct damage.

3. Equipment-corrosion path (attacker's [+0x92] bit 0x800/0x400
   picks which equipment slot -- 0x13A/0x142/0x146, the confirmed
   equipment-slot layout): a weaker FailsSavingThrow roll (half the
   normal DC stat); on a failed save, targets the DEFENDER's equipped
   item in that slot via GetClassifiedItemStatField instead of
   dealing HP damage -- a "corrode equipment" special attack, tying
   into the equipped-item durability system (TickEquippedItemDurability).

-> ResolveAttackerActionOutcome

Run via:
    .\run_ida_script.ps1 name_resolve_attacker_action_outcome.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16BF6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResolveAttackerActionOutcome", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResolveAttackerActionOutcome': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves one attacker-vs-defender action outcome, one of 3 "
    "paths selected by word_328CA bit 0x200 and the attacker's "
    "[+0x92] special-attack flags: (1) normal ResolveAttack damage "
    "roll, (2) a FailsSavingThrow-gated status-effect application, "
    "or (3) a weaker-DC FailsSavingThrow gating an 'equipment "
    "corrosion' effect that targets the defender's equipped item "
    "instead of HP. Called twice from sub_16881.",
    False,
)

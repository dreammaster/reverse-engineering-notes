"""
Names sub_2D428, called once from ApplyAttackAlongCorridorLine, right
after ApplyAttackToTarget, when any damage/status is still pending
(word_2E49C/word_2E49A nonzero).

Important context established while naming this: ApplyAttackToTarget
only ever leaves word_2E49C/word_2E49A nonzero by having already run
its OWN full commit (subtracting damage from [di+0x10], setting
[di+0xC]/[di+0x96]/[di+0x1C]/[di+0x1E]) -- every early-return path
explicitly re-checks both are 0 first. So this function runs as a
genuine *second* commit pass on the same target, not a fallback for a
skipped one.

What it does, precisely:
1. Re-filters word_2E49A by target immunity ([di+0x96]) and ORs it
   into [di+0xC] again (idempotent -- bits already set stay set).
2. Recomputes a *compounded* resistance halving: unlike
   ApplyTargetResistancesToAttack's single first-match halving, this
   loops all 16 bit positions and halves word_2E49C once per matching
   bit among the same 7 resistance-category bits (word_33306 & 0xFE00
   & [di+98h]) -- so an attack matching multiple resistance types gets
   halved multiple times here.
3. Subtracts this newly-recomputed amount from [di+0x10] *again*
   (floored at 0) -- i.e. genuinely double-applies damage on top of
   ApplyAttackToTarget's own subtraction.
4. Re-sets [di+0xC]|=3 and conditionally clears bit 0 (same condition
   as ApplyAttackToTarget's own tail) -- both idempotent.

Why the game deliberately re-applies damage this way for corridor-line
attacks specifically (as opposed to a bug, or an intentional
"compounding elemental damage" design for area effects) is NOT
resolved -- flagging this honestly rather than guessing.
-> ReapplyDamageWithCompoundedResistance

Run via:
    .\run_ida_script.ps1 name_reapply_compounded_resistance.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D428
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ReapplyDamageWithCompoundedResistance", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ReapplyDamageWithCompoundedResistance': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Second commit pass after ApplyAttackToTarget already committed: "
    "re-filters status flags by immunity (idempotent), recomputes a "
    "COMPOUNDED resistance halving (once per matching bit among the "
    "same 7 word_33306/[di+0x98] resistance-category bits, vs. "
    "ApplyTargetResistancesToAttack's single first-match halving), "
    "and subtracts that from [di+0x10] AGAIN (floored at 0) -- "
    "genuinely double-applies damage. Why this re-application is "
    "intentional (compounding elemental damage for area attacks?) "
    "vs. an artifact isn't resolved. Called from "
    "ApplyAttackAlongCorridorLine.",
    False,
)

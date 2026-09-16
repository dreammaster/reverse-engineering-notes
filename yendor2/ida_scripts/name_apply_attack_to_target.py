"""
Names sub_2D4B6, called twice from the large unnamed combat dispatcher
sub_2C0FE. Orchestrates a full attack-resolution-and-application pass
against one target (di):

1. Resolves the base damage: if word_328CA bit 0x80 is clear, calls
   TryResolveAttackAgainstTarget (which itself may skip the attack via
   its own guard); if set, either takes word_332E8 directly (unless
   the same word_33306-bit-0x100/[di+0x4E]-vs-word_332D8 guard blocks
   it) or -- when word_33306 bit 0x10 is set -- overrides the staged
   damage with [di+0x5A]/2 instead.
2. Calls ApplyTargetResistancesToAttack to filter the pending status
   flags and damage by the target's resistances/immunities.
3. If nothing survived (no damage and no status flags staged),
   returns without touching the target.
4. Otherwise commits the results to the target record: sets flag bits
   [di+0xC] |= 3, subtracts the final damage from [di+0x10] (a
   HP-like current-value field), and if any status flags survived,
   ORs them into [di+0xC] (and, gated on word_33300 bit 0x200, into
   [di+0x96] too), then overwrites [di+0x1C]/[di+0x1E] with fixed
   globals word_332EE/word_332FC -- structurally the same offsets as
   the confirmed party-record affliction bitfield, though `di`'s
   record type here is still unconfirmed (see the standing caution
   note for its two callees). Finally, if word_33306 bit 0x20 is set,
   clears bit 0 of [di+0xC].

In short: the per-target "resolve attack, apply resistances, commit
damage and status effects" pipeline. -> ApplyAttackToTarget

Run via:
    .\run_ida_script.ps1 name_apply_attack_to_target.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D4B6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyAttackToTarget", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyAttackToTarget': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves base damage (TryResolveAttackAgainstTarget, or a direct "
    "word_332E8/[di+0x5A]/2 path), filters it through "
    "ApplyTargetResistancesToAttack, then -- if any damage or status "
    "flags survived -- commits to the target: [di+0xC]|=3, "
    "[di+0x10]-=damage, ORs surviving status flags into [di+0xC] "
    "(and [di+0x96] if word_33300 bit 0x200), overwrites "
    "[di+0x1C]/[di+0x1E] with word_332EE/word_332FC, and conditionally "
    "clears [di+0xC] bit 0. Called twice from sub_2C0FE.",
    False,
)

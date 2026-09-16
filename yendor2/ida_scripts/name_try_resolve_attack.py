"""
Names sub_2D171, called once from unnamed sub_2D4B6 (itself part of
the large unnamed combat dispatcher sub_2C0FE's tree): a small guard
around ResolveAttack.

If word_33306 bit 0x100 is set AND a per-target field ([di+0x4E])
already equals the global word_332D8, returns without attacking
(presumably "this target was already resolved this round/pass" --
word_332D8's exact role is not confirmed: it's also read/compared
against small constants (9, 0xD) elsewhere, and against a
"ShowClueBookSpellDetail+0x1F6" xref that looks like an IDA
tail-chunk artifact rather than real logical overlap, so it may be a
reused scratch value rather than single-purpose). Otherwise calls
ResolveAttack(ax=[di+0x58], bx=[si+0x62], cx=word_332E8) where
si=word_328D4 (the current/selected party member). Note: `di`'s
record type (attacker vs. target, party vs. monster) isn't confirmed
here, so [di+0x4E]/[di+0x58] are NOT assumed to be the same-named
party-record fields documented elsewhere at those offsets.
-> TryResolveAttackAgainstTarget

Run via:
    .\run_ida_script.ps1 name_try_resolve_attack.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D171
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryResolveAttackAgainstTarget", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryResolveAttackAgainstTarget': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Skips the attack if word_33306 bit 0x100 is set and [di+0x4E] "
    "already equals word_332D8 (plausibly 'already resolved this "
    "round'). Otherwise calls ResolveAttack(ax=[di+0x58], "
    "bx=[si+0x62], cx=word_332E8), si=word_328D4. Field identities at "
    "di's offsets not confirmed -- di's record type here is unknown. "
    "Called from sub_2D4B6.",
    False,
)

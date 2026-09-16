"""
Names sub_2D195, called once from the still-unnamed combat dispatcher
sub_2C0FE: calls ResolveAttack([di+0x58], [current-party-record+0x62],
cx=word_332E8), then -- only if word_2E49C is still 0 (a one-time
latch) -- sets errorCode=1 and stores word_332E8 into word_2E49C.
Reads as "resolve this attack, and latch word_332E8 the first time
through" (e.g. recording which attack/monster index first connects),
but the exact field identities ([di+0x58], party record +0x62,
word_332E8's meaning) aren't confirmed -- sub_2C0FE itself remains
untraced. -> ResolveAttackAndLatchFirstHit

Run via:
    .\run_ida_script.ps1 name_resolve_attack_latch.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D195
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResolveAttackAndLatchFirstHit", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResolveAttackAndLatchFirstHit': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Calls ResolveAttack, then latches word_332E8 into word_2E49C the "
    "first time through (only if word_2E49C was still 0), setting "
    "errorCode=1. Field identities ([di+0x58], party record +0x62, "
    "word_332E8) not confirmed. Called once from the still-unnamed "
    "sub_2C0FE.",
    False,
)

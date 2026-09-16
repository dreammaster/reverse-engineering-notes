"""
Names sub_182CE, called from ApplyEffectAndDrawIconBar: applies a
generic capped/floored stat delta via the icon-bar slot mechanism (si
= icon-bar slot, di = a related pointer whose [+0xA] flags select the
behavior).

If [di+0xA] bit 0x100: adds [si+0xE] to a party-record field (selected
by [si+0x10], base [si+0xC]), capped at another field selected by
[si+0x12].
If bit 0x80 instead: subtracts [si+0xE] from a field (base + [si+0x10]),
floored at 0.
Either way (unless word_33306 bits 0xC0 are set), clears bits from the
party record's +0x1C status word (masked by word_332E2), then finishes
with sub_1AA9B, UpdatePartyAverageStatTiers, and -- notably --
CheckForLevelUp. The level-up check strongly suggests at least one use
of this is a gradual/staged XP-granting effect, though the exact field
identity (data-driven via the icon-bar slot, not hardcoded here) isn't
confirmed. -> ApplyIconBarStatDelta

Run via:
    .\run_ida_script.ps1 name_apply_iconbar_stat_delta.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x182CE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyIconBarStatDelta", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyIconBarStatDelta': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Applies a capped (bit 0x100) or floored (bit 0x80) stat delta via "
    "the icon-bar slot (fields selected by data, not hardcoded), clears "
    "a +0x1C status bit range, then finishes with "
    "UpdatePartyAverageStatTiers + CheckForLevelUp -- the level-up "
    "check suggests a gradual/staged XP-like effect, but the specific "
    "field isn't confirmed. Called from ApplyEffectAndDrawIconBar.",
    False,
)

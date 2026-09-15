"""
Traced sub_17795 (called from both UseAbilityCommand and
HandleMovementInput) and dumped its message strings directly from the
data segment. It's the lock-examination message shower -- and its
strings are the EXACT 7-tier key hierarchy (BRASS/BRONZE/COPPER/IRON/
STEEL/SILVER/GOLD KEY) already cross-confirmed early in the session
against the Hex Hacking Item Guide's door-key item table (see
file-formats.md's Item-slot encoding section) -- this function is
where that string survey finding actually gets used in code.

Branches (gated on word_32DCE's bits and the current party member's
[+0x6C] field, compared against ASCII-looking thresholds 0x37/'7',
0x41/'A', 0x50/'P' -- plausibly a lockpicking/perception skill value):
"NOT LOCKED", "LOCKED", "MAGICALLY LOCKED" (word_32DCE bit 0x20),
"LOCKED AND TRAPPED" (if skill high enough to detect the trap), or
"REQUIRES SPECIAL KEY: <tier> KEY" (word_32DCE bits 0x200-0x8000
select which of the 7 tiers, or a generic "SPECIAL KEY" fallback) --
gated on skill thresholds that determine how much detail is revealed.

-> ShowLockStatus

Run via:
    .\run_ida_script.ps1 name_show_lock_status.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17795
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowLockStatus", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowLockStatus': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Lock-examination message shower (called from UseAbilityCommand "
    "and HandleMovementInput). Shows 'NOT LOCKED'/'LOCKED'/"
    "'MAGICALLY LOCKED' (word_32DCE bit 0x20)/'LOCKED AND TRAPPED', "
    "or 'REQUIRES SPECIAL KEY: <tier> KEY' -- the exact 7-tier key "
    "hierarchy (BRASS/BRONZE/COPPER/IRON/STEEL/SILVER/GOLD, "
    "word_32DCE bits 0x200-0x8000) already cross-confirmed early in "
    "the session against the Hex Hacking Item Guide's door-key item "
    "table. Gated on the current party member's [+0x6C] field "
    "(plausibly a lockpicking/perception skill) against thresholds.",
    False,
)

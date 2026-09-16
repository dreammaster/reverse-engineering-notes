"""
Names sub_1B5FD, called from UseItem's "item [+0xE] flag 0x400" path
(the "UseItemType_400" path already referenced informally in
file-formats.md's party-record notes).

Checks a one-time-use global flag (TestGlobalFlag on a value read
from a fixed table at 0xBCE+0x12) and does nothing if already
consumed. Otherwise shows "EXPERIENCE:" (confirmed via string dump at
0x7DBB) followed by "+<BCD4 value>" (the amount at 0xBCE+0x14) in
the status panel, sets the one-time flag, then loops over all 4
party slots (table 0x95EB) and, for each occupied slot whose record
doesn't have the +0x1C bit 0x40 affliction flag set, adds that same
BCD4 amount directly to the character's +0x18 field -- confirmed
elsewhere (file-formats.md) as packed-BCD experience points -- and
calls CheckForLevelUp, then sets up an icon-bar slot entry for the
grant. Finishes with UpdatePartyAverageStatTiers and
ApplyEffectAndDrawIconBar. A one-time-use "tome of experience" style
consumable item. -> UseExperienceBoostItem

Run via:
    .\run_ida_script.ps1 name_use_experience_boost_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B5FD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseExperienceBoostItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseExperienceBoostItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One-time-use 'tome of experience' item effect (UseItem's item "
    "[+0xE] bit 0x400 path). Gated on a global one-time-use flag "
    "(0xBCE+0x12); shows 'EXPERIENCE: +<BCD4 amount>' (0xBCE+0x14) "
    "and adds that amount to every eligible living party member's "
    "+0x18 XP field, then CheckForLevelUp per member.",
    False,
)

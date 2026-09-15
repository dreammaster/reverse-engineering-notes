"""
Traced sub_28FF9, AdvanceGameClock's "new day" handler (fired when the
minutes-since-midnight counter overflows past 1440). For each of the
4 party members, zeroes their [+0xB6] through [+0xBC] fields -- the
exact 4 special-ability charge fields from the RevealMapRegion/
UseAbilityScroll system, many rounds ago. Confirms special abilities
recharge once per in-game day, a classic "daily use" cooldown.

-> ResetDailyAbilityCharges

Run via:
    .\run_ida_script.ps1 name_reset_daily_charges.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28FF9
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResetDailyAbilityCharges", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResetDailyAbilityCharges': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "AdvanceGameClock's 'new day' handler: for each of the 4 party "
    "members, zeroes [+0xB6]/[+0xB8]/[+0xBA]/[+0xBC] -- the 4 "
    "special-ability charge fields (see RevealMapRegion/"
    "UseAbilityScroll). Special abilities recharge once per in-game "
    "day.",
    False,
)

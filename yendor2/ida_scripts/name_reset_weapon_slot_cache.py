"""
Names sub_1DC73, called from HandleRangedOrCombatAction: only runs
every Nth call (decrements word_2E544, acting only when it hits 0).
Picks an x-offset (0x36/0x69/0x9D) matching the first of
word_328D8/word_328DA/word_328DC that is zero -- these are the exact
same globals and x-positions HandleRangedOrCombatAction's own code
feeds to DrawWeaponSelectIcon for its 4-icon ranged-weapon selection
bar (slots at x=0/0x36/0x69/0x9D) -- then clears (fills with 0xFFFF) a
105-row region of a dedicated EMS page (0x55FE, distinct from the
corridor/portrait page 0x55D8) at that x-offset. Reads as resetting a
cached per-weapon-slot data region (ammo count display? projectile
animation cache?) for whichever weapon slot is currently empty.
-> ResetWeaponSlotDisplayCache

Run via:
    .\run_ida_script.ps1 name_reset_weapon_slot_cache.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1DC73
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ResetWeaponSlotDisplayCache", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ResetWeaponSlotDisplayCache': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Every Nth call (word_2E544 countdown), clears a 105-row region of "
    "EMS page 0x55FE at the x-offset of the first empty weapon-select "
    "slot (word_328D8/DA/DC, the same globals/positions "
    "HandleRangedOrCombatAction feeds to DrawWeaponSelectIcon). "
    "Plausibly resets a per-slot display/animation cache. Called from "
    "HandleRangedOrCombatAction.",
    False,
)

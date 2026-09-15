"""
Names sub_233F5, called from SpawnMonsterInFacingDirection and
FindMonsterTypeInLevelPool: a per-monster "has the party gotten close
enough to notice this monster" activation check.

If the monster's [+0xC] bit 0 (an "activated/aware" flag) is already
set, does nothing. Otherwise compares the current render-depth
counter (word_3292C) against a threshold that depends on the
monster's own detection-range flags at [+0x94] (0x20/0x40/0x80/0x100
select increasingly close thresholds -- 0x21/0x2C/0x29/0x26) -- once
the depth counter clears the relevant threshold, sets [+0xC] bit 0,
marking the monster as active/aware.

-> TryActivateMonsterByDistance

Run via:
    .\run_ida_script.ps1 name_monster_activation_check.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x233F5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryActivateMonsterByDistance", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryActivateMonsterByDistance': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If not already active ([+0xC] bit 0), checks the render-depth "
    "counter (word_3292C) against a per-monster detection-range "
    "threshold selected by [+0x94] flags (0x20/0x40/0x80/0x100 -> "
    "0x21/0x2C/0x29/0x26) -- sets [+0xC] bit 0 once close enough, "
    "marking the monster active/aware. Called from "
    "SpawnMonsterInFacingDirection and FindMonsterTypeInLevelPool.",
    False,
)

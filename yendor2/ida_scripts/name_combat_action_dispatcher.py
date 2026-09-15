"""
Names sub_1D4B8, called from `start` (2 sites: one setting word_328C8
bit 0x100 first, one not) and reached from a formal-combat check
inside itself. The overall combat-action entry point, branching three
ways:

1. If already in formal combat (word_328CA bit 0x1000): jumps to
   loc_1D902 (not traced this round).
2. Else if word_328C8 bit 0x100 is set (the caller's "ranged attack"
   flag): the fully-traced **ranged weapon shot sequence** --
   scans the 4 party inventory slots for characters with an eligible
   ranged weapon (item 0x13A, status-flag gated), bails if none; else
   draws a 4-icon weapon-select UI, redraws the screen, then animates
   a projectile traveling down the corridor one depth row at a time
   (AnimateProjectileStep + ClassifyObstacleAtViewportRow, rows
   0x31/0x2E/0x2B/0x28/0x24/0x19) until it hits something. On a wall/
   door (errorCode 1/2), shows a "deflected" message (sub_1DA42).
   On a monster (errorCode 4), resolves the attack via
   ResolveAttackOrAbilityAction and shows a hit/miss follow-up based
   on word_328C8 bit 0x200.
3. Else (bit 0x100 clear, not in combat): jumps to loc_1D77B -- a
   parallel spell/ability-cast opening sequence, not traced this
   round.

-> HandleRangedOrCombatAction

Run via:
    .\run_ida_script.ps1 name_combat_action_dispatcher.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D4B8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleRangedOrCombatAction", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleRangedOrCombatAction': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Combat-action entry point, 3-way branch: formal combat "
    "(word_328CA bit 0x1000, not traced), a fully-traced ranged-weapon "
    "shot sequence (word_328C8 bit 0x100 set -- select weapon, animate "
    "a projectile down the corridor row by row via "
    "AnimateProjectileStep/ClassifyObstacleAtViewportRow, resolve via "
    "ResolveAttackOrAbilityAction on a monster hit), or a "
    "spell/ability-cast opening (bit 0x100 clear, not traced). Called "
    "from `start`.",
    False,
)

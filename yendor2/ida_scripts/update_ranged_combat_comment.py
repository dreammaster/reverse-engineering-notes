"""
Updates HandleRangedOrCombatAction's comment now that its other two
branches (in-combat melee, area-effect spell finish) are understood.

Run via:
    .\run_ida_script.ps1 update_ranged_combat_comment.py
"""
import ida_bytes

ea = 0x1D4B8
ida_bytes.set_cmt(
    ea,
    "Combat-action entry point, 3-way branch on word_328CA bit 0x1000 "
    "(in formal combat) and word_328C8 bit 0x100 (ranged-attack "
    "request): (1) in-combat melee -- HighlightSelectedAbilityIcon, "
    "one AnimateProjectileStep, then ResolveAttackOrAbilityAction "
    "directly against word_32A1E (no row search, target already "
    "known); (2) ranged-weapon shot -- select weapon, animate a "
    "projectile down the corridor row by row via "
    "AnimateProjectileStep/ClassifyObstacleAtViewportRow, resolve on a "
    "monster hit; (3) spell/ability cast when not in combat -- mostly "
    "the same shape as (2). A successful area-effect spell (2 special "
    "ability ids) plays a 10-frame explosion animation then sweeps all "
    "80 g_levelMonsters slots for kills, not just the rows touched. "
    "Called from `start`.",
    False,
)
print("comment updated")

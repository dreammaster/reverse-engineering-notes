"""
Final update to HandleRangedOrCombatAction's comment: the
spell/ability-cast branch is now fully traced too (it converges into
the same row-scan code as the ranged branch), completing the picture.

Run via:
    .\run_ida_script.ps1 finalize_combat_action_comment.py
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
    "known); (2) ranged-weapon shot and (3) spell/ability cast (not "
    "in combat) -- these converge into the identical code: select "
    "weapon/ability, animate a projectile down the corridor row by row "
    "via AnimateProjectileStep/ClassifyObstacleAtViewportRow, resolve "
    "via ResolveAttackOrAbilityAction on a hit, with a multi-shot "
    "continuation (word_2E544) for characters with more than one "
    "attack. A successful area-effect spell plays a 10-frame explosion "
    "animation then sweeps all 80 g_levelMonsters slots for kills. "
    "Common epilogue for all 3 branches: loot-staging check -> "
    "ShowLootAndAwardExperience -> ProcessLevelMonsters -> redraw. "
    "Called from `start`.",
    False,
)
print("comment finalized")

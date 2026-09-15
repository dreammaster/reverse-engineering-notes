"""
Names sub_25656, called from DrawMonsterAndUpdateAttackState (when the
0x3010 attack-state bits are clear) and from ShowClueBookMonsterDetail.
Advances the monster's current animation frame ([+8], the picture id
DrawMonsterAndUpdateAttackState draws) within a small cycle relative
to a per-monster base frame ([+0x4C]), selecting between two cycle
modes (walk-style vs. idle-style) via [+0x92] bits 0x20/0x10, with an
outright skip if bit 0x40 is set. Reused by the clue book's monster
detail screen to animate the preview sprite the same way.

-> AdvanceMonsterAnimationFrame

Run via:
    .\run_ida_script.ps1 name_advance_monster_animation.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25656
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AdvanceMonsterAnimationFrame", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AdvanceMonsterAnimationFrame': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Advances a monster's idle/walk animation frame ([+8]) within a "
    "small cycle relative to a base frame ([+0x4C]), mode selected by "
    "[+0x92] bits 0x20/0x10 (skipped entirely if bit 0x40 set). Called "
    "from DrawMonsterAndUpdateAttackState (non-attacking case) and "
    "ShowClueBookMonsterDetail (animates the clue-book preview the "
    "same way).",
    False,
)

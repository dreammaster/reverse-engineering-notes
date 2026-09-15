"""
Names sub_20E54, called from TryTriggerMonsterEncounterAtCell (after a
monster is confirmed present, whether just-spawned or already there)
and from RenderActiveMonsterSprites (per active g_monsterSlots
monster). Draws the monster's sprite in the dungeon viewport and
updates its attack-readiness state.

Mechanism:
- Optional blit-mask setup ([+0x92] bit 2 -> g_blitMaskPtr/Len) for a
  see-through/dithered draw style.
- Draws the monster's base picture ([+8]) at z-layer [+0xA], with a
  wound-flash override: if [+0xC] bit 2 is set, shows a "just hit"
  frame ([+0x4C]+9) once and clears the bit; if bit 4 is set and the
  current frame is behind the "recovering" frame ([+0x4C]+6), advances
  to it and updates [+8] -- a hit-flash/recovery animation.
- If [+0xC] bit 0x10 is set, draws an extra overlay picture ([+0x1A]).
- Draws a weapon/attack-effect sprite at one of 5 fixed screen
  positions selected by word_32918 (9-13), offset by [+0x68]/[+0x6A],
  picking a picture id from word_329E8/EA/EC based on [+0xE] flags
  0x2000/0x4000 (melee vs. ranged-style variants).
- Finally: if [+0xC] has any of bits 0x3010 set (the same flags
  ProcessLevelMonsters documents as gating TickMonsterTimer's
  two-phase countdown), resets the countdown and sets bit 2 (marks
  "just attacked", presumably); otherwise calls sub_25656 (not traced
  -- plausibly the monster's actual attack-resolution step).

-> DrawMonsterAndUpdateAttackState

Run via:
    .\run_ida_script.ps1 name_draw_monster_sprite.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x20E54
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMonsterAndUpdateAttackState", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMonsterAndUpdateAttackState': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a monster's sprite in the dungeon viewport (base picture, "
    "wound-flash animation via [+0xC] bits 2/4, optional overlay via "
    "bit 0x10, plus a weapon/attack-effect sprite), then checks [+0xC] "
    "bits 0x3010 (same flags ProcessLevelMonsters documents for "
    "TickMonsterTimer's two-phase countdown): if set, resets the "
    "countdown; else calls sub_25656 (not traced, plausibly attack "
    "resolution). Called from TryTriggerMonsterEncounterAtCell and "
    "RenderActiveMonsterSprites.",
    False,
)

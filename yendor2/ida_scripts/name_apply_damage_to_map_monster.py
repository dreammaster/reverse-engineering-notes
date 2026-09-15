"""
Names sub_2D370, called twice from sub_2C0FE (a large, unnamed
dispatcher -- also referenced from RunAlchemyScreen's exit path via
ApplyMapTriggerEffect). Applies damage to a monster standing in the
dungeon corridor (a g_levelMonsters-pool monster, not yet one of the
3 g_monsterSlots turn-based combatants) and resolves the outcome.

If there's damage to apply (word_2E49A + word_2E49C != 0): computes it
via sub_2D498, sets a wound-tier field ([+0x18]) and display flags
([+0xC] |= 3, conditionally clearing bit 0 per word_33306 bit 0x20),
redraws (RefreshDungeonScreen), waits 5 ticks, then checks the
monster's HP ([+0x10]): if <= 0, calls the already-named
GrantMonsterRewards + RemoveMonsterFromMap + RedrawDungeonScreen
(death); otherwise just RefreshDungeonScreen again (survives).

-> ApplyDamageToMapMonster

Run via:
    .\run_ida_script.ps1 name_apply_damage_to_map_monster.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D370
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyDamageToMapMonster", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyDamageToMapMonster': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Applies damage (word_2E49A+word_2E49C) to a dungeon-corridor "
    "monster (g_levelMonsters, via sub_2D498/sub_2D4B6, not traced), "
    "sets wound/display flags, redraws and waits, then resolves death "
    "(GrantMonsterRewards + RemoveMonsterFromMap + RedrawDungeonScreen) "
    "or survival (RefreshDungeonScreen) based on HP ([+0x10]). Called "
    "from sub_2C0FE.",
    False,
)

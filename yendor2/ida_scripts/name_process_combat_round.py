"""
Traced sub_16B63 (called directly from RunDungeonGameLoop) -- the
missing piece connecting HandleDungeonInput's HP subtraction to actual
monster death handling and turn advancement.

For each occupied g_monsterSlots entry: if its HP ([+0x10]) is <= 0,
finds the matching g_combatTurnOrder entry and sets its flag 0x4000
-- **confirms the "plausibly defeated" guess for that bit from several
rounds ago**. Clears word_32A1E if the dead monster was the active
target, then calls GrantMonsterRewards (stages its loot into the
global counters, as already traced) and sub_2313D (not traced this
round, presumably the matching g_monsterSlots-entry cleanup).

If every monster is still alive this pass, it instead advances the
turn: ensures word_32A1E is set (via SelectActiveMonster if needed),
calls sub_2333B (not traced), then walks g_combatTurnOrder from
word_32BF4 forward looking for the next entry that isn't flagged
0x4000 (defeated) -- classic "find the next living combatant in turn
order" advancement.

-> ProcessCombatRound

Run via:
    .\run_ida_script.ps1 name_process_combat_round.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16B63
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ProcessCombatRound", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ProcessCombatRound': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "For each occupied g_monsterSlots entry with HP ([+0x10]) <= 0: "
    "flags its g_combatTurnOrder entry 0x4000 (confirms 'defeated'), "
    "clears word_32A1E if it was the active target, and calls "
    "GrantMonsterRewards + sub_2313D (cleanup, not traced). If none "
    "died this pass, advances the turn instead: ensures word_32A1E is "
    "set (SelectActiveMonster), calls sub_2333B (not traced), then "
    "walks g_combatTurnOrder from word_32BF4 for the next "
    "non-defeated entry.",
    False,
)

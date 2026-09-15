"""
Followed RunDungeonGameLoop's other monster-related callee, sub_22D4C
(also called directly from `start`), and found the level-wide monster
pool that feeds the 3-slot g_monsterSlots combat system found last
round.

sub_22D4C iterates a MUCH larger table: si=0xF26, cx=0x50 (80), stride
0x9C -- the SAME 156-byte record stride as g_monsterSlots, strongly
suggesting the same record layout. For each occupied slot (`[si+0xC] &
1`), it calls sub_22CED (a per-monster timer/state-machine tick) and
branches on the returned errorCode: 0 = nothing to do, 1 = the monster
just became ready to act (calls sub_22B96 then sub_23116 -- not traced,
plausibly "spawn this monster into an active g_monsterSlots combat
slot"). This reads as the per-level monster spawn/wander pool, distinct
from (and feeding into) the 3 simultaneous *active-combat* slots.
-> ProcessLevelMonsters. The table itself -> g_levelMonsters.

sub_22CED (the per-monster tick): gated on `[si+0xC] & 0xFC10` (an
"is this monster active at all" flag test). Decrements a countdown
([si+0x10] -= [si+0x1C], a per-tick speed/rate value) -- if it reaches
zero, errorCode=1 ("ready/arrived"); for monsters flagged 0x3010 in
[si+0xC] (the same flag combo BuildCombatTurnOrder tests -- plausibly
"aggressive/hostile" monsters), does a second decrement pass with an
intermediate errorCode=2 signal before falling through. Separately,
decrements [si+0x1E] (a slower, secondary countdown) and when *that*
reaches zero, resets the monster's state wholesale: clears several
[si+0xC] flag bits, zeroes [si+0x1A]/[si+0x1C]/[si+0x1E], and restores
[si+8] from a template value at [si+0x4C] -- classic "monster killed/
expired, prepare to respawn" reset. -> TickMonsterTimer.

Run via:
    .\run_ida_script.ps1 name_monster_spawn_system.py
"""
import idc
import ida_name
import ida_bytes

FUNC_RENAMES = {
    0x22D4C: "ProcessLevelMonsters",
    0x22CED: "TickMonsterTimer",
}

for ea, name in FUNC_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

# 0xF26 is a DS-relative immediate; linear = ds_base + 0xF26.
ds_base = 0x2D860
data_ea = ds_base + 0xF26
old = idc.get_name(data_ea)
ok = ida_name.set_name(data_ea, "g_levelMonsters", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{data_ea:#x}  {old!r} -> 'g_levelMonsters': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x22D4C,
    "Iterates g_levelMonsters (80 x 0x9C-byte records, same stride as "
    "g_monsterSlots) -- for each occupied slot ([si+0xC] & 1), calls "
    "TickMonsterTimer and, on errorCode==1 ('ready'), calls sub_22B96 "
    "then sub_23116 (not traced, plausibly promotes this monster into "
    "an active g_monsterSlots combat slot). Also does an unrelated "
    "IsBCDCounterAtLeast(0x51B6) check + sub_23151 at the end (see "
    "RunDungeonGameLoop, same pairing).",
    False,
)
ida_bytes.set_cmt(
    0x22CED,
    "Per-monster timer/state-machine tick (si = a g_levelMonsters "
    "entry). Gated on [si+0xC] & 0xFC10. Decrements [si+0x10] by "
    "[si+0x1C]; reaching 0 sets errorCode=1 ('ready/arrived'). "
    "Monsters flagged 0x3010 in [si+0xC] (same combo "
    "BuildCombatTurnOrder tests) get a second decrement pass with an "
    "intermediate errorCode=2. Separately, decrements [si+0x1E] and "
    "on reaching 0 resets the monster wholesale (clears [si+0xC] "
    "flag bits 0x3ED, zeroes [si+0x1A]/[si+0x1C]/[si+0x1E], restores "
    "[si+8] from a template at [si+0x4C]) -- plausibly a death/respawn "
    "reset.",
    False,
)
ida_bytes.set_cmt(
    data_ea,
    "80 x 0x9C-byte monster records (per-level monster pool, feeding "
    "the 3-slot g_monsterSlots active-combat array). Same record "
    "stride and [+0xC] flag conventions as g_monsterSlots.",
    False,
)

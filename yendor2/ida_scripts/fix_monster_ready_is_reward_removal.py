"""
CORRECTION: last round's ProcessLevelMonsters/TickMonsterTimer comments
guessed that TickMonsterTimer's errorCode==1 ("ready") "plausibly
promotes this monster into an active g_monsterSlots combat slot" via
the two untraced callees. Read both this round -- wrong guess.

sub_22B96(si=g_levelMonsters entry): adds 4 fixed BCD values (from
0x51BA/0x5396/0x539A, plus the global counter at 0x51B6 itself) into
the monster's own [+0x7E]/[+0x82]/[+0x86] fields and into 0x51B6 (the
SAME global BCD counter ProcessLevelMonsters/RunDungeonGameLoop check
via IsBCDCounterAtLeast to conditionally call sub_23151) -- reads as
granting a reward (gold/treasure/score) both to a per-monster tally and
a global running total. Then adjusts two more stat deltas ([si+0x14]/
[si+0x16], sign-dispatched to sub_27A46/sub_27A2A, not traced).
-> GrantMonsterRewards

sub_23116(si=g_levelMonsters entry): clears a "present on this map
cell" flag (bit 0x400 of the map cell's own +6 field, via a pointer at
the monster's [+6]) and zeroes that map cell's [+4] (an "occupant"
reference), then zeroes the ENTIRE monster record (cx=0x4E words =
0x9C bytes, the full record size). This is monster removal/cleanup --
confirms the errorCode==1 path is "this monster's presence has ended:
grant its reward, then wipe it from both the map and the record pool."
-> RemoveMonsterFromMap

So TickMonsterTimer's [+0x10] countdown is better read as a monster's
remaining-presence/lifespan timer (or possibly specifically a "corpse/
loot" delay) rather than a movement-readiness timer as guessed last
round -- reaching 0 ends the monster's presence with a reward grant,
not a combat-readiness signal. Corrected the comments on
ProcessLevelMonsters and TickMonsterTimer to match.

Run via:
    .\run_ida_script.ps1 fix_monster_ready_is_reward_removal.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x22B96: "GrantMonsterRewards",
    0x23116: "RemoveMonsterFromMap",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x22B96,
    "Grants a reward on this monster's presence ending: adds 4 fixed "
    "BCD values (0x51BA/0x5396/0x539A, plus the global counter at "
    "0x51B6 itself) into the monster's [+0x7E]/[+0x82]/[+0x86] and "
    "into the global BCD counter 0x51B6 (checked elsewhere via "
    "IsBCDCounterAtLeast to conditionally call sub_23151 -- plausibly "
    "an achievement/threshold notification). Also adjusts two signed "
    "stat deltas ([+0x14]/[+0x16] -> sub_27A46/sub_27A2A, not traced).",
    False,
)
ida_bytes.set_cmt(
    0x23116,
    "Removes a monster from the map and wipes its record: clears the "
    "'present here' flag (bit 0x400) on the map cell its [+6] field "
    "points at and zeroes that cell's [+4] occupant reference, then "
    "zeroes the entire g_levelMonsters/g_monsterSlots-layout record "
    "(0x9C bytes).",
    False,
)
ida_bytes.set_cmt(
    0x22D4C,
    "Iterates g_levelMonsters (80 x 0x9C-byte records, same stride as "
    "g_monsterSlots) -- for each occupied slot ([si+0xC] & 1), calls "
    "TickMonsterTimer and, on errorCode==1 (this monster's presence "
    "has ended), calls GrantMonsterRewards then RemoveMonsterFromMap. "
    "Also does an unrelated IsBCDCounterAtLeast(0x51B6) check + "
    "sub_23151 at the end (see RunDungeonGameLoop, same pairing).",
    False,
)
ida_bytes.set_cmt(
    0x22CED,
    "Per-monster timer/state-machine tick (si = a g_levelMonsters "
    "entry). Gated on [si+0xC] & 0xFC10. Decrements [si+0x10] "
    "(plausibly a remaining-presence/lifespan timer, not confirmed "
    "movement-related) by [si+0x1C]; reaching 0 sets errorCode=1, "
    "which callers (ProcessLevelMonsters) treat as 'this monster's "
    "presence has ended' -- granting a reward and removing it. "
    "Monsters flagged 0x3010 in [si+0xC] (same combo "
    "BuildCombatTurnOrder tests) get a second decrement pass with an "
    "intermediate errorCode=2. Separately, decrements [si+0x1E] and "
    "on reaching 0 resets the monster wholesale (clears [si+0xC] "
    "flag bits 0x3ED, zeroes [si+0x1A]/[si+0x1C]/[si+0x1E], restores "
    "[si+8] from a template at [si+0x4C]) -- plausibly preparing the "
    "slot for reuse/respawn.",
    False,
)

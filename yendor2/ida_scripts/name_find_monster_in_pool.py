"""
Names sub_22B78 and corrects a mistake in TryTriggerMonsterEncounterAtCell's
comment from last round.

sub_22B78 (-> FindMonsterTypeInLevelPool): scans g_levelMonsters
(0xF26, 80 x 0x9C) for an entry whose [+0] matches the given monster
type id (ax). If found, calls sub_233F5 (the same setup call
SpawnMonsterInFacingDirection makes) and returns "found" (ZF clear);
if not found, returns "not found" (ZF set, si=0).

CORRECTION: TryTriggerMonsterEncounterAtCell's comment described this
call as "a probability check" -- wrong. Re-reading the call site
(`mov ax,[di+4]; call sub_22B78; jnz loc_212E7`), it's actually a
**duplicate-prevention lookup**: if a monster of this type already
exists somewhere in g_levelMonsters, skip spawning another one --
only if NOT already present does it fall through to
SpawnMonsterInFacingDirection. This reads as unique/boss-monster
dedup, not a random encounter chance roll.

Run via:
    .\run_ida_script.ps1 name_find_monster_in_pool.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22B78
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FindMonsterTypeInLevelPool", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FindMonsterTypeInLevelPool': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Scans g_levelMonsters for an entry matching the given monster "
    "type id (ax). Found -> sub_233F5 + ZF clear; not found -> ZF set. "
    "Used by TryTriggerMonsterEncounterAtCell as a duplicate-prevention "
    "check before spawning (skips spawning if this type already exists "
    "on the level) -- NOT a probability roll, correcting last round's "
    "comment.",
    False,
)

ida_bytes.set_cmt(
    0x212B8,
    "Per-cell encounter check: only fires for word_3292C >= 0x11 (the "
    "farthest visible rows) and a flag bit on the cell record "
    "([di+6] bit 0x400); skips spawning if this monster type already "
    "exists on the level (FindMonsterTypeInLevelPool -- CORRECTION: "
    "not a probability roll as first described), then calls "
    "SpawnMonsterInFacingDirection. Called once per cell from "
    "RenderDungeonViewRow.",
    False,
)

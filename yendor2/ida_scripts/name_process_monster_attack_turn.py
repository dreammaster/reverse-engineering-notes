"""
Names sub_16881, called once from RunDungeonGameLoop -- fully
resolves now that its caller context is read closely: RunDungeonGameLoop
tests g_combatTurnOrder's current entry ([bx+6] bit 0x8000, the
confirmed "this entry is a monster" flag) and calls sub_16881 when
it's a monster's turn (calling HandleDungeonInput instead for a
party member's turn). This is the per-monster combat-turn processor.

si = [bx] = the turn-order entry's record pointer (a g_monsterSlots
record) = word_32904, the attacker. Sequence:

1. TickMonsterTimer, then if the monster's [+0xC] behavior flags
   have any of 0xF010 set, flags itself "shown" (or 0x20) and briefly
   displays DrawMonsterInfoPanels (a reveal/intro pause).
2. If errorCode was set by TickMonsterTimer, bails immediately.
3. di = [+0x12] = the monster's assigned target (confirmed field) =
   word_32908, the defender; sets [+0xC] bit 4 (an "attacking" flag)
   and flashes the dungeon view 3 times.
4. Branches on the monster's own [+0x92] bit 0x1000 -- a genuinely
   new find, an "area-effect attack" flag:
   - If set: loops all 4 party slots (table 0x95EB), skipping
     incapacitated members ([+0x1C] bits 0x1C40), calling
     SelectTrapEffectVariant + ResolveAttackerActionOutcome against
     each -- a breath-weapon/group-attack monster hitting the whole
     party in one turn, playing its hit sound ([+0x5C]) only once
     (word_328C8 bit 4 guard).
   - If clear (the common case): single-target attack against the
     assigned defender (skipped if incapacitated, falling through to
     an idle/grumble sound instead): computes the defender's icon-bar
     slot (word_32906 = 0xC50 + partySlot*0x14, the confirmed icon-
     bar layout), calls SelectTrapEffectVariant +
     ResolveAttackerActionOutcome, and on a successful hit plays the
     hit sound, sets word_32DC0 = [+0x52] (a save-DC stat) for
     ApplySavingThrowEffect, and applies/draws the icon-bar effect.
     Then, unless word_328CA bit 0x200 is set, calls
     TickEquippedItemDurability(0x146) on the defender -- the
     monster's attack can wear/break the defender's equipped item in
     the array-item slot, tying directly into this session's earlier
     TickEquippedItemDurability/ResolveAttackerActionOutcome findings.
5. The idle/grumble fallback (no valid target): plays [+0x5E] (a
   distinct "idle" sound id) if the sound driver is ready, else waits
   12 ticks.
6. Common exit: advances g_combatTurnOrder's cursor bookkeeping and
   refreshes the dungeon screen.

-> ProcessMonsterAttackTurn

Run via:
    .\run_ida_script.ps1 name_process_monster_attack_turn.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16881
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ProcessMonsterAttackTurn", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ProcessMonsterAttackTurn': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Per-monster combat-turn processor, called from RunDungeonGameLoop "
    "when g_combatTurnOrder's current entry is a monster's turn. "
    "Ticks the monster, shows its info panel on first reveal, then "
    "either attacks all 4 party members (if [+0x92] bit 0x1000 is "
    "set -- an area-effect/breath-weapon monster) or its single "
    "assigned target ([+0x12]) via ResolveAttackerActionOutcome, "
    "optionally wearing/breaking the defender's equipped item "
    "(TickEquippedItemDurability) on a hit. Falls back to an idle "
    "sound if no valid target.",
    False,
)

"""
Names sub_2333B, called once from ProcessCombatRound after a death
pass completes and SelectActiveMonster has picked the new active
monster (word_32A1E).

Operates on the confirmed g_monsterSlots pool (base 0x51C0, 3x0x9C
records, file-formats.md's "Combat: monster slots and turn order"
section): checks which of the 3 slots are occupied ([+0]!=0) and,
based on the occupancy pattern, picks a source/destination pair among
the 3 fixed slot addresses (0x51C0/0x525C/0x52F8), incrementing or
decrementing a per-slot counter at [+0xA] as it does so. Copies the
full 156-byte monster record from source to destination (via a
scratch buffer), zeroes the vacated source slot, then scans the
first 7 entries of g_combatTurnOrder (0x539E, 8-byte stride, [+0]=
record pointer) for one still pointing at the old (pre-move) address
and rewrites it to the new one. Reads as: after a monster dies and
its slot goes empty, shift the remaining live monsters into a
contiguous front-loaded arrangement and fix up any stale turn-order
pointer left over from the move. -> CompactMonsterSlots

Run via:
    .\run_ida_script.ps1 name_compact_monster_slots.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2333B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CompactMonsterSlots", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CompactMonsterSlots': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "After a monster death pass, shifts the remaining live "
    "g_monsterSlots records into a contiguous front-loaded "
    "arrangement (picking source/dest among the 3 fixed slot "
    "addresses based on occupancy), then rewrites any "
    "g_combatTurnOrder entry that still points at the old address. "
    "Called once from ProcessCombatRound.",
    False,
)

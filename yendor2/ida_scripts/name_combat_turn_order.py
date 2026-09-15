"""
Traced sub_16A39 (called every RunDungeonGameLoop iteration, right
before the "3 fixed records" display) and found the combat turn-order
system -- which also finally nails down what those "3 fixed records"
at 0x51C0/0x525C/0x52F8 actually are: **monster slots**, not generic
status widgets (correcting last round's hedge again, this time with
solid evidence).

sub_16A39 -> BuildCombatTurnOrder: builds a combined combatant list
at 0x539E (8 bytes/entry, up to 4 party + 3 monster slots = 7 max) --
for each valid party member (via g_partySlotAssignment), stores its
record ptr, its party-slot address (via FindPartySlotForRecord/
sub_16D4D), and a speed/initiative value from [+0x3E]; for each
occupied monster slot (0x51C0/0x525C/0x52F8, confirmed a 3-entry,
0x9C/156-byte-stride array -- g_monsterSlots), stores its record ptr,
a speed value from [+0x56], and marks it with flag 0x8000 (vs. party
entries, which never get this bit) -- and, unless already flagged
0x2000, has it pick a random living party member to target
(RandomInRange(3) into g_partySlotAssignment, validated against the
0x1C40 status mask, stored into the monster's own [+0x12]). Then
insertion-sorts the whole list by the speed value (descending) --
classic initiative ordering. Finally, if no monster is currently
active (word_32A1E==0), calls SelectActiveMonster.

sub_16D4D -> FindPartySlotForRecord(ax=combatant record ptr): scans
g_partySlotAssignment for a party slot matching this record; if found,
returns its slot address in word_32924 (and bx); if not found (i.e.
the record is a monster, not a party member), resets word_328D4/
word_328D6 to 0.

sub_16D7F -> SelectActiveMonster: scans the sorted turn-order list for
the first entry flagged 0x8000 (a monster) and NOT flagged 0x4000
(plausibly "defeated"), sets word_32A1E to its record pointer (0 if
none found, i.e. no monsters left active).

Data: 0x51C0 (linear via ds) -> g_monsterSlots (3 x 0x9C-byte monster
records; confirmed fields so far: +0xC type/behavior flags tested
against 0x3010, +0x12 current target (a party record pointer, set by
BuildCombatTurnOrder's random-target pick), +0x56 speed/initiative).
0x539E -> g_combatTurnOrder (14 x 8-byte scratch list, rebuilt every
loop iteration: +0 record ptr, +2 party-slot addr or 0 for monsters,
+4 speed value, +6 flags: 0x8000=monster, 0x2000=?, 0x4000=plausibly
defeated).

sub_232A8/sub_234D3 (the "3 fixed records" display, previously hedged
as unclear) are now understood to be drawing the 3 monster slots'
info panels, gated by the word_36CA9 party-average "identify" tier
from two rounds ago -- named accordingly.

Run via:
    .\run_ida_script.ps1 name_combat_turn_order.py
"""
import idc
import ida_name
import ida_bytes

FUNC_RENAMES = {
    0x16A39: "BuildCombatTurnOrder",
    0x16D4D: "FindPartySlotForRecord",
    0x16D7F: "SelectActiveMonster",
    0x232A8: "DrawMonsterInfoPanels",
    0x234D3: "DrawMonsterInfoPanel",
}

DATA_RENAMES = {
    0x32A20: "g_monsterSlots",   # 0x51C0 + DS base 0x2D860
    0x32BFE: "g_combatTurnOrder",  # 0x539E + DS base 0x2D860
}

for ea, name in FUNC_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea, name in DATA_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x16A39,
    "Builds a combined party+monster turn-order list at "
    "g_combatTurnOrder (8 bytes/entry): party members via "
    "g_partySlotAssignment (+0=record, +2=party-slot addr via "
    "FindPartySlotForRecord, +4=speed from [+0x3E]); monsters via "
    "g_monsterSlots (+0=record, +4=speed from [+0x56], +6 flag "
    "0x8000 set, and unless already flagged 0x2000, picks a random "
    "living party target into the monster's [+0x12]). Insertion-sorts "
    "by speed. If no monster is active (word_32A1E==0), calls "
    "SelectActiveMonster.",
    False,
)
ida_bytes.set_cmt(
    0x16D4D,
    "FindPartySlotForRecord(ax=combatant record ptr): scans "
    "g_partySlotAssignment for a matching party slot; found -> "
    "word_32924/bx = that slot's address. Not found (record is a "
    "monster) -> resets word_328D4/word_328D6 to 0, word_32924=0.",
    False,
)
ida_bytes.set_cmt(
    0x16D7F,
    "Scans g_combatTurnOrder (cx=7, up to 4 party + 3 monster "
    "entries) for the first entry flagged 0x8000 (monster) and not "
    "0x4000 (plausibly defeated); sets word_32A1E to its record "
    "pointer, or 0 if none found (no monsters currently active).",
    False,
)
ida_bytes.set_cmt(
    0x232A8,
    "Draws the 3 g_monsterSlots info panels via DrawMonsterInfoPanel "
    "at 3 fixed screen positions (previously misidentified as generic "
    "'status widgets' before BuildCombatTurnOrder confirmed these "
    "addresses are monster records).",
    False,
)
ida_bytes.set_cmt(
    0x234D3,
    "Draws one monster's info panel (si = g_monsterSlots entry): name "
    "strings, then progressively more detail icons as the party's "
    "average word_36CA9 stat (an 'identify'-style tier) crosses 3 "
    "thresholds, selected by 2-bit quality flags on the monster's "
    "own [+0xC] field.",
    False,
)
ida_bytes.set_cmt(
    0x32A20,
    "3 x 0x9C-byte monster/combatant records (linear 0x32A20 = "
    "0x51C0 + ds base). Confirmed fields: +0xC type/behavior flags "
    "(tested against 0x3010 in BuildCombatTurnOrder), +0x12 current "
    "target (a party record pointer), +0x56 speed/initiative value.",
    False,
)
ida_bytes.set_cmt(
    0x32BFE,
    "14 x 8-byte combat turn-order scratch list, rebuilt every "
    "RunDungeonGameLoop iteration by BuildCombatTurnOrder. +0 record "
    "ptr, +2 party-slot address (0 for monsters), +4 speed/initiative "
    "(sort key, descending), +6 flags (0x8000=monster, 0x2000=?, "
    "0x4000=plausibly defeated).",
    False,
)

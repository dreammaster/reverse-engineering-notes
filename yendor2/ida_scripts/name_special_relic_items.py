"""
Traced and dumped message strings for the remaining 4 items in the
item-icon-dispatch cluster (0x246-0x249), gated by TestGlobalFlag(0xB1)
("PATIENCE IS A VIRTUE" if not yet available) -- powerful, quest/
relic-tier effects, part of the same themed cluster as
ShowVisionAtLocation/UseLocationBoundPotion/CheckQuestItemsCompleted.

sub_2B029 -> CollectNuoreCache (item 0x246): "+5,000 NUORE", adds
5000 to the global material counter 0x94BB via AddToBCDCounter.

sub_2AFB8 -> CollectMagicOreCache (item 0x247): "+5,000 MAGIC ORE",
adds 5000 to counter 0x94B7.

sub_2B09A -> PartyMassHealAndOverheal (item 0x248): "2 X HEALTH" /
"2 X MAGIC" -- cures all ailments (clears [+0x1C] low 6 bits) and sets
EVERY party member's current HP/MP to 2x their max (an overheal/
blessing effect exceeding the normal cap), showing a heal icon
(PrepareTrapEffectSlots effect id 3, same as UseHealingItem) on each.

sub_2B14F -> InstantKillActiveMonster (item 0x249, additionally gated
on word_328CA bit 0x1000): zeroes the active monster's HP
([word_32A1E+0x10] = 0) directly -- an instant-kill effect.

Run via:
    .\run_ida_script.ps1 name_special_relic_items.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2B029: "CollectNuoreCache",
    0x2AFB8: "CollectMagicOreCache",
    0x2B09A: "PartyMassHealAndOverheal",
    0x2B14F: "InstantKillActiveMonster",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2B029,
    "Item-icon-dispatch handler (word_32974==0x246). Shows "
    "'+5,000 NUORE', confirms item 0x246 present "
    "(IsItemRangeAvailable), adds 5000 to global material counter "
    "0x94BB.",
    False,
)
ida_bytes.set_cmt(
    0x2AFB8,
    "Item-icon-dispatch handler (word_32974==0x247). Shows "
    "'+5,000 MAGIC ORE', confirms item 0x247 present, adds 5000 to "
    "global material counter 0x94B7.",
    False,
)
ida_bytes.set_cmt(
    0x2B09A,
    "Item-icon-dispatch handler (word_32974==0x248). Shows "
    "'2 X HEALTH'/'2 X MAGIC': cures all ailments and sets every "
    "party member's current HP/MP to 2x their max (an overheal "
    "effect), drawing a heal icon (PrepareTrapEffectSlots id 3, same "
    "as UseHealingItem) on each via ApplyEffectAndDrawIconBar.",
    False,
)
ida_bytes.set_cmt(
    0x2B14F,
    "Item-icon-dispatch handler (word_32974==0x249, also gated on "
    "word_328CA bit 0x1000). Zeroes the active monster's HP "
    "([word_32A1E+0x10]=0) directly -- an instant-kill effect.",
    False,
)

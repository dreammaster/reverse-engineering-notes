"""
Names sub_149DD and sub_148EA -- ShowArmorDetailRow's two bonus-list
sub-drawers, each iterating up to 4 entries at word_2E54A ([+0]=type
id, [+2]=amount) and printing "+<amount> <name>" for each nonzero
entry, using a different lookup table depending on the type-id range.

sub_149DD (-> ShowArmorProtectionsList): "PROTECTIONS:" (msg 0x7B24)
for type ids <= 0x30, name from a 9-entry table at 0x7B31: DISEASE,
POISON, SICKNESS, STONING, FROZEN, PARALYZE, CURSING, HEXING,
JINXING -- the item's affliction-protection bonuses.

sub_148EA (-> ShowArmorAttributeBonusList): "ADDS:" (msg 0x8AAB) for
type ids >= 0x7C, name from a 27-entry table at 0x7DC7 -- the full
attribute/skill name list: STRENGTH, DEXTERITY, STAMINA, INTELLIGENCE,
WISDOM, CHARISMA, (3 blank slots), HIT POINTS, MAGIC POINTS, (blank),
SURVIVAL, PROJECTILE, SLASHING, BASHING, POLEARM, CASTING, MAPPING,
NAVIGATION, BARTERING, REPAIR, THIEVERY, LINGUISTICS, CHEMISTRY, ...
-- a major reference find: the canonical index order of the game's
attribute/skill system, previously only known piecemeal from string
scans.

Run via:
    .\run_ida_script.ps1 name_armor_bonus_lists.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x149DD: "ShowArmorProtectionsList",
    0x148EA: "ShowArmorAttributeBonusList",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x149DD,
    "'PROTECTIONS:' list for word_2E54A entries with type id <= 0x30: "
    "DISEASE/POISON/SICKNESS/STONING/FROZEN/PARALYZE/CURSING/HEXING/"
    "JINXING (table 0x7B31). Called from ShowArmorDetailRow.",
    False,
)
ida_bytes.set_cmt(
    0x148EA,
    "'ADDS:' attribute/skill bonus list for word_2E54A entries with "
    "type id >= 0x7C: table 0x7DC7 is the canonical 27-entry attribute/"
    "skill name list (STRENGTH..LINGUISTICS and beyond). Called from "
    "ShowArmorDetailRow.",
    False,
)

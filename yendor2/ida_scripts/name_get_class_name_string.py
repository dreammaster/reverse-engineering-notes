"""
Names sub_19768 and sub_2BFBC, and corrects ShowWorldMap's comment.

sub_19768 (0x19768): given a class id (ax, 1-27) in the piecewise
"cmp 9 / sub 0xA" pattern already documented for +0xE (class id), returns
a pointer (bx) into one of two contiguous string tables (11-byte
stride). Dumped the actual bytes -- it's a real class-name table:
  1 FIGHTER, 2 MERCHANT, 3 ROGUE, 4 MONK, 5 ALCHEMIST, 6 PALADIN,
  7 MAGE, 8 DRUID, 9 MARKSMAN, 10 WARRIOR, 11 TINKERER, 12 THIEF,
  13 CLERIC, 14 TRANSMUTER, 15 CAVALIER, 16 WIZARD, 17 ENCHANTER,
  18 RANGER, 19 CHAMPION, 20 BLACKSMITH, 21 ASSASSIN, 22 PRIEST,
  23 HEALER, 24 HERO, 25 SORCERER, 26 SAGE, 27 KNIGHT
This directly confirms +0xE is a class id (previously "plausibly").
-> GetClassNameString

sub_2BFBC (0x2BFBC): called only from ShowWorldMap. For one party
record (si=word_328D4-style base, di=screen position pair): draws an
icon at (di[0],di[4]) with picture id [si+0x12] (word_2E530), sets font
transparent, writes the name string ([si+0], di advanced by 0xA), then
calls GetClassNameString([si+0xE]) and writes that too (di advanced by
another 0xA). I.e. draws one "[icon] NAME (CLASS)" roster row.
-> DrawPartyRosterEntry

Also corrects ShowWorldMap's own comment: it iterates the 9-slot
g_partyRecords array (0x95F3, stride 0x1F4) calling DrawPartyRosterEntry
per occupied slot (+0x16 != 0), not "location markers" as originally
guessed -- +0x16 is the already-documented level/skill field, not a
discovered-location flag, and DrawPartyRosterEntry proves each row shows
a character's name+class, not a town/location. Digit-key selection
(1-9) and a flag-toggle-and-list-removal path (+0x15C bit 0x800, two
small tables at 0x95EB/0x94A3) look like a recruit/dismiss roster.

Run via:
    .\run_ida_script.ps1 name_get_class_name_string.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x19768, "GetClassNameString"),
    (0x2BFBC, "DrawPartyRosterEntry"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19768,
    "Given a class id (ax, 1-27), returns a pointer (bx) into one of two "
    "contiguous 11-byte-stride string tables -- a real class-name table "
    "(FIGHTER/MERCHANT/ROGUE/MONK/ALCHEMIST/PALADIN/MAGE/DRUID/MARKSMAN "
    "for 1-9; WARRIOR/TINKERER/THIEF/CLERIC/TRANSMUTER/CAVALIER/WIZARD/"
    "ENCHANTER/RANGER/CHAMPION/BLACKSMITH/ASSASSIN/PRIEST/HEALER/HERO/"
    "SORCERER/SAGE/KNIGHT for 10-27). Confirms +0xE is a class id.",
    False,
)

ida_bytes.set_cmt(
    0x2BFBC,
    "Draws one party-roster row for a record (si): icon at [si+0x12], "
    "name string at [si+0], and class name via GetClassNameString([si+0xE]). "
    "Called only from ShowWorldMap, once per occupied roster slot.",
    False,
)

ida_bytes.set_cmt(
    0x2BD1A,
    "Moderate-high confidence, corrected: NOT a location-marker map "
    "overlay (original guess). Draws g_pictureDir entry 4 full-screen, "
    "then iterates the 9-slot g_partyRecords array (base 0x95F3, stride "
    "0x1F4) drawing one roster row per occupied slot (+0x16 != 0 -- the "
    "already-documented level/skill field, used here as an "
    "occupied-slot check, not a 'discovered' flag) via "
    "DrawPartyRosterEntry (icon + name + class name -- proves these are "
    "characters, not towns). Digit keys 1-9 (and a second, differently- "
    "routed key range) select a slot by index and call sub_23C18 to open "
    "a detail/interaction screen; one path toggles a flag (+0x15C bit "
    "0x800) and removes the slot's index from two small lookup tables "
    "(0x95EB/0x94A3) when set -- plausibly a recruit/dismiss roster "
    "screen (add/remove a character from the active adventuring group), "
    "not a set of townsite markers. Not fully traced: sub_23C18, the "
    "toggle's exact meaning, and the digit-vs-alt-key distinction remain "
    "open.",
    False,
)

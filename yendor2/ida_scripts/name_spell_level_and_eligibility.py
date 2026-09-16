"""
Names two more helpers behind ShowClueBookSpellDetail's spell-detail
screen, both built on DrawLabeledNumberRow.

sub_13C4B, called once from ShowClueBookSpellDetail: searches a
20-level x 2-class-slot table (di walks 2 candidate class ids per
level, bp counts the level) for a match against word_3330A (the
current class id). On a match, draws the matched level (ax=bp) via
DrawLabeledNumberRow with a fixed "LEVEL:" label string (0x8CDD) and
highlight color 0x8A; sets errorCode=1 if no level in the table
matches this class at all. -> DrawSpellLevelForCurrentClass

sub_13C1D, called once from ShowClueBookSpellDetail (a different call
site, presumably looped 6x for the documented "6-class eligibility
marker row"): checks a 2-entry candidate array ([bp+0]/[bp+2]) against
word_3330A; on a match, draws a fixed value of 1 via
DrawLabeledNumberRow with highlight color 0xA7 and a fixed string
(0x8D83) -- marking this class slot as eligible for the spell.
-> DrawClassEligibilityMarker

Run via:
    .\run_ida_script.ps1 name_spell_level_and_eligibility.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x13C4B: "DrawSpellLevelForCurrentClass",
    0x13C1D: "DrawClassEligibilityMarker",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x13C4B,
    "Searches a 20-level x 2-class-slot table for a match against "
    "word_3330A (current class id); on a match, draws the level via "
    "DrawLabeledNumberRow ('LEVEL:' label, color 0x8A). errorCode=1 "
    "if no match. Called from ShowClueBookSpellDetail.",
    False,
)
ida_bytes.set_cmt(
    0x13C1D,
    "Checks a 2-entry candidate array against word_3330A; on a "
    "match, draws a fixed '1' via DrawLabeledNumberRow (color 0xA7) "
    "-- a class-eligibility marker, plausibly one cell of the "
    "documented 6-class eligibility row. Called from "
    "ShowClueBookSpellDetail.",
    False,
)

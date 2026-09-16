"""
Names sub_24D30, sub_250BB, and sub_256F0 -- the character sheet's main
stat-column renderer, found investigating a ranked candidate
(sub_250BB) and tracing up to its caller.

sub_256F0 (0x256F0, called from FormatAndDrawBCD4 and others): strips
all space characters from a string in place (compacting it) --
a generic string utility. -> StripSpaces

sub_250BB (0x250BB, called twice from sub_24D30): a sibling of
DrawValueWithThresholdColor (same ax>bx threshold-color logic) but
using StripSpaces instead of sub_2570C after FormatNumber -- a
"trimmed" variant. Called here for the HP row ([+0x52]/[+0x92]) and MP
row ([+0x54]/[+0x94]). -> DrawTrimmedThresholdValue

sub_24D30 (0x24D30, called from ShowCharacterSkills and the
still-untraced sub_23C18): the character sheet's main stat renderer.
Left column (x=0xCB): the 6 core attributes (+0x3C.. +0x46, via
DrawValueWithThresholdColor) immediately followed, in the same column,
by the +0x4C/+0x4E/+0x50 trio (already named via DrawThreeThresholdStats
and confirmed NOT part of the 6 core attributes -- but visually
positioned right after them, slots 7-9 in the same list), then HP and
MP rows (DrawTrimmedThresholdValue) and packed-BCD XP
(FormatAndDrawBCD4). Right column (x=0x129): 13 contiguous derived
stats (+0x58.. +0x70, the block ComputeDerivedCharacterStats computes),
the last 5 of which (+0x68/+0x6A/+0x6C/+0x6E/+0x70) are highlighted
when this character's roster slot number matches one of 5 global
"assigned role" slots (word_36D03/36D05/36D07/36D09/36D0B) --
consistent with practical party-role skills (navigator, mapper,
barterer, etc.) where one character is the active specialist. Finishes
with an averaged-attribute tier message (only for slot 1) via
sub_23BA4 (not traced). -> DrawCharacterStatSheet

Run via:
    .\run_ida_script.ps1 name_char_stat_sheet.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x256F0, "StripSpaces"),
    (0x250BB, "DrawTrimmedThresholdValue"),
    (0x24D30, "DrawCharacterStatSheet"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x256F0,
    "Strips all space characters from a string in place (compacting "
    "it). Called from FormatAndDrawBCD4 and others.",
    False,
)
ida_bytes.set_cmt(
    0x250BB,
    "Sibling of DrawValueWithThresholdColor (same ax>bx threshold-color "
    "logic) but using StripSpaces after FormatNumber instead of "
    "sub_2570C. Called from DrawCharacterStatSheet for the HP and MP "
    "rows.",
    False,
)
ida_bytes.set_cmt(
    0x24D30,
    "Character sheet's main stat renderer: left column draws the 6 "
    "core attributes plus the +0x4C/+0x4E/+0x50 trio (same column, "
    "slots 7-9), then HP/MP rows and packed-BCD XP; right column draws "
    "13 derived stats (+0x58..+0x70), the last 5 highlighted when this "
    "character holds one of 5 globally-assigned party roles "
    "(word_36D03/05/07/09/0B). Called from ShowCharacterSkills and "
    "sub_23C18.",
    False,
)

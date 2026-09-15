"""
Traced sub_252EF (called from ShowCharacterStats and ShowCharacterSkills,
i.e. during character creation/display) and it finally maps the party
record's 6 core attributes -- flagged "not yet mapped" since the very
start of this session's file-formats.md work.

For a freshly-created character (si=word_328D4), rolls 6 attributes,
each RandomInRange(15)+45 (45-59), storing each into a "base" field and
a paired "derived/current" field at +0x40 higher offset:
  +0x3C -> +0x7C (also *10 into +0x56/+0x96 -- a weight-like derived
           stat; matches STRENGTH determining carry capacity)
  +0x3E -> +0x7E (no secondary derived use found in this function)
  +0x42 -> +0x82 (the [+0x82] UseTrainingItem blends into its MP-growth
           formula -- matches INTELLIGENCE, a classic MP-driving stat)
  +0x44 -> +0x84 (the [+0x84] UseTrainingItem's other MP-growth
           component -- matches WISDOM, the other classic MP stat)
  +0x46 -> +0x86 (used in UseTrainingItem's separate growth calc,
           bx=0xD divisor -- not matched to a specific attribute)
  +0x40 -> +0x80, and additionally +0x80 scaled by 25%
           (ScaleByPercentRounded) sets both current and max HP
           (+0x52/+0x92) -- matches STAMINA/CONSTITUTION determining HP

So +0x82/+0x84 (already known from UseTrainingItem's MP formula) and
+0x80 (HP formula) are now confirmed to be 3 of the 6 core attributes,
not just "some class-table values" as documented 2 rounds ago. Exact
attribute-name assignment for all 6 isn't independently confirmed
(the roll order doesn't obviously match the manual's STR/DEX/STA/INT/
WIS/CHA listing), so named the function on its confirmed mechanism
rather than asserting specific attribute names.

-> RollCharacterAttributes

Run via:
    .\run_ida_script.ps1 name_roll_character_attributes.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x252EF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RollCharacterAttributes", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RollCharacterAttributes': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Rolls the 6 core attributes for word_328D4 (RandomInRange(15)+45 "
    "each, 45-59), storing base+derived field pairs: +0x3C/+0x7C "
    "(also x10 into +0x56/+0x96, weight-like -- plausibly STRENGTH); "
    "+0x3E/+0x7E; +0x42/+0x82 (MP-formula component in "
    "UseTrainingItem -- plausibly INTELLIGENCE); +0x44/+0x84 (the "
    "other MP-formula component -- plausibly WISDOM); +0x46/+0x86 "
    "(a separate UseTrainingItem growth calc); +0x40/+0x80, whose "
    "25%-scaled value sets both current and max HP (+0x52/+0x92) -- "
    "plausibly STAMINA/CONSTITUTION.",
    False,
)

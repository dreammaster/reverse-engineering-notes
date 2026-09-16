"""
Names sub_25CFA, called from UseItem and UseAbilityScroll: only runs
if the character has learned at least one special ability (+0xB4, the
confirmed "learned abilities" bitmask). Draws a header
(DrawCharacterNameHeader), then for each of the 4 possible abilities
(bit test on +0xB4) draws its name from the table at 0x77C6 (already
documented as checked against thresholds before RevealMapRegion and
its 3 sibling abilities) in bright (ready) or dim (not ready) color --
"ready" requires the charge/level field (+0xB6/+0xB8/+0xBA/+0xBC,
individually already documented) to meet the table's threshold AND
(for some abilities) the current time of day (word_36D01) to fall in a
day or night window, per flag bits in the table. -> DrawAbilityReadinessList

Run via:
    .\run_ida_script.ps1 name_ability_readiness_list.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25CFA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAbilityReadinessList", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAbilityReadinessList': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If the character has learned any special ability (+0xB4 nonzero), "
    "draws each learned ability's name (table 0x77C6) in bright/dim "
    "color depending on whether its charge field (+0xB6/+0xB8/+0xBA/"
    "+0xBC) meets the table's threshold and, for some abilities, "
    "whether the current time of day (word_36D01) is in the required "
    "window. Called from UseItem and UseAbilityScroll.",
    False,
)

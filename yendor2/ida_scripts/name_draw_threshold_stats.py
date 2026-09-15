"""
Names sub_24CAD and sub_25091, called from sub_23C18 and the
already-named ShowCharacterSkills: draws 3 rows of a
threshold-highlighted stat value.

sub_25091 (-> DrawValueWithThresholdColor): draws a formatted number
(ax) using a highlight color (word_2E414) if ax > bx (a threshold),
else the default color (word_2E412).

sub_24CAD (-> DrawThreeThresholdStats): draws 3 such values from a
party record (si=word_328D4): [+0x4C] vs threshold [+0x8C],
[+0x4E] vs [+0x8E], [+0x50] vs [+0x90] -- three fields immediately
before the confirmed HP (`+0x52`/`+0x92`) and MP (`+0x54`/`+0x94`)
pairs, plausibly 3 of the character's primary attributes (Strength/
Dexterity/Stamina, going by the canonical attribute-table order found
via `ShowArmorAttributeBonusList`) but not confirmed which is which.

Run via:
    .\run_ida_script.ps1 name_draw_threshold_stats.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x25091: "DrawValueWithThresholdColor",
    0x24CAD: "DrawThreeThresholdStats",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25091,
    "Draws a formatted number (ax), using a highlight color if ax > "
    "bx (a threshold). Called from DrawThreeThresholdStats.",
    False,
)
ida_bytes.set_cmt(
    0x24CAD,
    "Draws 3 threshold-highlighted stat values from the current party "
    "record: [+0x4C]/[+0x8C], [+0x4E]/[+0x8E], [+0x50]/[+0x90] -- "
    "immediately before the confirmed HP/MP field pairs, plausibly 3 "
    "primary attributes (not confirmed which). Called from sub_23C18 "
    "and ShowCharacterSkills.",
    False,
)

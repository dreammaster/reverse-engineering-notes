"""
Names sub_25FCD, a major find: called from sub_25B34 (the panel-select
routine). Draws a header (DrawCharacterNameHeader), the label
"PROTECTIONS:", then 9 rows pairing a resistance-type name (table at
0x7B31, via sub_23B76) with its numeric value read from the character
record starting at +0x20 -- CONFIRMING, by name, the previously-guessed
"9 contiguous 2-byte equipment/bonus resistance values" (+0x20..+0x30,
found via RollEffectResistance) as, in order:
  +0x20 DISEASE, +0x22 POISON, +0x24 SICKNESS, +0x26 STONING,
  +0x28 FROZEN, +0x2A PARALYZE, +0x2C CURSING, +0x2E HEXING,
  +0x30 JINXING
This matches the manual's affliction list exactly (with "SICKNESS"
filling out the 9th slot) and resolves the "individual fields not yet
matched to specific resistance types" open note.
-> DrawCharacterProtectionsList

Run via:
    .\run_ida_script.ps1 name_character_protections_list.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25FCD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawCharacterProtectionsList", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawCharacterProtectionsList': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws 'PROTECTIONS:' plus 9 rows pairing DISEASE/POISON/SICKNESS/"
    "STONING/FROZEN/PARALYZE/CURSING/HEXING/JINXING (table 0x7B31) with "
    "their values at +0x20..+0x30 -- confirms, by name, the "
    "'9 contiguous resistance values' found via RollEffectResistance. "
    "Called from sub_25B34.",
    False,
)

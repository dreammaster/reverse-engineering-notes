"""
Names sub_2498B: a text-entry field (sub_1D1D4, 13-char max, same shape
as the save-name entry in RunGameDialog's Save flow) captures typed
text; if non-empty after trimming, copies it via StpCpy into
word_328D4+0 (the party-member record's very first field) and displays
it. Setting/editing a character's name. -> EditCharacterName

Run via:
    .\run_ida_script.ps1 name_char_rename.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2498B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "EditCharacterName", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'EditCharacterName': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Text-entry field (13-char max) for the character's name -- "
    "non-empty result gets copied into word_328D4+0 (the party-member "
    "record's first field) and displayed. ShowPartyMembers pipeline "
    "step.",
    False,
)

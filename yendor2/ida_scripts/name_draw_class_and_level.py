"""
Names sub_2504F: draws the character-sheet header "class name + level"
for the current character (si=word_328D4). Calls GetClassNameString and
writes it, then draws [si+0x16] via DrawValueWithThresholdColor with
ax==bx (both [si+0x16]) so the highlight branch never fires -- just a
plain number draw reusing that helper. Called from ShowCharacterSkills
and sub_23C18 (still-untraced), i.e. shared by multiple character-sheet
screens. This is further confirmation +0x16 is the character's level
(already documented via FailsSavingThrow's save-chance formula and
UseTrainingItem's level-up cap of 90) rather than a generic "skill"
value. -> DrawCharacterClassAndLevel

Run via:
    .\run_ida_script.ps1 name_draw_class_and_level.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2504F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawCharacterClassAndLevel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawCharacterClassAndLevel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws the current character's (word_328D4) class name "
    "(GetClassNameString) and level (+0x16, via DrawValueWithThresholdColor "
    "with ax==bx so no highlight ever fires -- a plain number draw). "
    "Shared by ShowCharacterSkills and sub_23C18.",
    False,
)

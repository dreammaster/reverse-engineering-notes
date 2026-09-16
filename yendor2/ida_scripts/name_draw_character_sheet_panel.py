"""
Names sub_254CC, called from both ShowCharacterStats and
ShowCharacterSummary -- the shared full character-sheet screen
assembly: draws the full-screen background (picture id 3, cached to
EMS), a title label, the party member's portrait
(DrawPartyMemberPortrait), a class/status picture at (0x74,0x13)
selected by the character record's [+0x12] field, then the stat
sheet (DrawCharacterStatSheet), the three threshold stats
(DrawThreeThresholdStats), class/level (DrawCharacterClassAndLevel),
and finally the character's name text. -> DrawCharacterSheetPanel

Run via:
    .\run_ida_script.ps1 name_draw_character_sheet_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x254CC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawCharacterSheetPanel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawCharacterSheetPanel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shared character-sheet screen assembly: full-screen background "
    "(picture 3) + title, portrait, a class/status picture selected "
    "by the character's [+0x12] field, then DrawCharacterStatSheet + "
    "DrawThreeThresholdStats + DrawCharacterClassAndLevel + name. "
    "Called from ShowCharacterStats and ShowCharacterSummary.",
    False,
)

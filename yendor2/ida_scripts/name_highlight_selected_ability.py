"""
Names sub_1D937, called from HandleRangedOrCombatAction (both the
ranged-shot opening and the in-combat melee branch). Bails if no
ability is selected (word_32974==0); otherwise highlights the
selected ability's icon in the 4-slot action UI (x/word_32980 set to
one of 0/0x36/0x69/0x9D based on which ability id matches).

-> HighlightSelectedAbilityIcon

Run via:
    .\run_ida_script.ps1 name_highlight_selected_ability.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D937
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HighlightSelectedAbilityIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HighlightSelectedAbilityIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Highlights the currently selected ability (word_32974) in the "
    "4-slot action UI, if any is selected. Called from "
    "HandleRangedOrCombatAction.",
    False,
)

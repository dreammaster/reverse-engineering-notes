"""
Names sub_29461, called from UseAbilityCommand and UseAbilityOnTarget
-- shows a multi-line text column (plausibly an ability's description
or effect text) at a fixed screen position, with cursor/panel
housekeeping around it.

Clears the status panel if dirty, sets word_328C4 bit 0x100 (an
"active display" flag), positions text at (0xF0, 0x60) with color
0x8A (the same highlight color DrawSpellLevelForCurrentClass uses)
and a transparent background, restores the cursor background if
dirty, then calls DrawStringColumn with the caller's own bx/cx
(preserved across the setup) -- the actual multi-line text-column
renderer (itself still unnamed). Finishes with DrawMouseCursor and
still-unnamed sub_238CD (a general cursor-position-sync routine, too
broad to name confidently this round). -> ShowAbilityDescriptionColumn

Run via:
    .\run_ida_script.ps1 name_show_ability_description_column.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29461
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowAbilityDescriptionColumn", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowAbilityDescriptionColumn': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears the status panel if dirty, sets word_328C4 bit 0x100, "
    "positions text (0xF0,0x60) color 0x8A transparent, restores the "
    "cursor background if dirty, then calls DrawStringColumn with the "
    "caller's bx/cx (preserved through setup) -- a multi-line text "
    "column, plausibly an ability's description. Called from "
    "UseAbilityCommand and UseAbilityOnTarget.",
    False,
)

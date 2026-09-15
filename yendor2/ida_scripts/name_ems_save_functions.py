"""
Names sub_2BB97 and sub_2BB1A -- EMS-backed background *save*
functions, the mirror image of RestoreCorridorBackgroundFromEMS
(which copies EMS -> video buffer; these copy video buffer -> EMS).

sub_2BB97 (-> SaveCorridorBackgroundToEMS): same region/dimensions as
RestoreCorridorBackgroundFromEMS (offset 0xA08, 136x112 words) but
copies video buffer -> EMS page frame -- caches the corridor viewport
before drawing an animated overlay on top of it. Called from
HandleRangedOrCombatAction.

sub_2BB1A (-> SaveActionIconPanelToEMS): a different screen region
(offset 0,0; 105x105 words, stride 0x6E) -- caches the action-icon
panel area. Called from HandleRangedOrCombatAction and
HighlightSelectedAbilityIcon.

Run via:
    .\run_ida_script.ps1 name_ems_save_functions.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2BB97: "SaveCorridorBackgroundToEMS",
    0x2BB1A: "SaveActionIconPanelToEMS",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2BB97,
    "Caches the corridor viewport background (video buffer -> EMS page "
    "frame, same region as RestoreCorridorBackgroundFromEMS) before an "
    "animated overlay draws over it. Called from "
    "HandleRangedOrCombatAction.",
    False,
)
ida_bytes.set_cmt(
    0x2BB1A,
    "Caches the action-icon panel area (video buffer -> EMS page "
    "frame, offset 0,0). Called from HandleRangedOrCombatAction and "
    "HighlightSelectedAbilityIcon.",
    False,
)

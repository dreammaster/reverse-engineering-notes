"""
Traced sub_178A6, called directly from `start` -- the top-level
"use an ability on a target" command shell (distinct from UseItem,
and distinct from the already-named UseAbilityOnTarget/ExamineTarget
pair from the 0xDFBB discovery-mechanic system early in the session,
though clearly related -- this looks like the outer wrapper that
picks/validates a target and persists the result, likely calling into
that mechanic internally via paths not fully traced this round).

Flow: dispatches on a caller-supplied record's +2 flags -- bit 0x8000
routes through sub_1766x (the weight/capacity check already flagged
from TryInteractAtPosition's docs), otherwise loads the target's
CURGAME record (LoadCurgameRecord). Confirms via ShowConfirmPrompt
(prompt 5 or 6, chosen by a flag), validates the target isn't
dead/invalid (status mask 0x1C40, else FlashStatusWarning and retry
target selection). Then, gated on a couple of state flags
(word_32DCE/word_32DC8/byte_32DCD), writes the result to CURGAME
(FileEntry bx=0x8FFB, errorCode=0xA) and shows a message box. Finally,
if the action record's own +2 bit 0x8000 was set, calls
ShowMaterialCounterHud (via sub_1732B) -- explains that HUD element's
calling context left open last round.

-> UseAbilityCommand

Run via:
    .\run_ida_script.ps1 name_use_ability_command.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x178A6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseAbilityCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseAbilityCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Top-level 'use ability on a target' command, called directly "
    "from `start`. Dispatches on a caller-supplied record's +2 flags "
    "(bit 0x8000 -> weight/capacity check via sub_1766F, else "
    "LoadCurgameRecord for the target). Confirms via ShowConfirmPrompt, "
    "validates the target (status mask 0x1C40, else "
    "FlashStatusWarning), writes the result to CURGAME (FileEntry "
    "0x8FFB) when gated flags allow, and shows a message box. If the "
    "action record's +2 bit 0x8000 was set, ends by calling "
    "ShowMaterialCounterHud (via sub_1732B) -- explains that HUD "
    "element's calling context.",
    False,
)

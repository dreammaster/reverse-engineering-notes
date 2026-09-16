"""
Names sub_216F0: calls FindObjectAtPosition(ax, bx) directly (caller
computes the target position -- `start`'s main loop uses this for
mouse-click-driven interaction, offsetting by a facing/click direction
much like ProbeFacingTile does for the keyboard path). If nothing's
there, errorCode=0. If something is, branches on its type flags
([si+2]): one type goes through a weight/capacity check
(sub_1766F) before succeeding, another loads its CURGAME record
(LoadCurgameRecord), others set specific errorCode failure reasons
(4, etc.). The caller uses the resulting errorCode to decide whether to
autosave state back to CURGAME. The core "try to interact with
whatever's at this map position" validator. -> TryInteractAtPosition

Run via:
    .\run_ida_script.ps1 name_try_interact.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x216F0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryInteractAtPosition", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryInteractAtPosition': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Validates an interaction/move at (ax, bx) via FindObjectAtPosition. "
    "Nothing there -> errorCode=0. Something there -> branches on its "
    "type flags ([si+2]): weight/capacity check (sub_1766F), "
    "LoadCurgameRecord, or specific failure codes. Caller (`start`'s "
    "main loop) uses the resulting errorCode to decide whether to "
    "autosave to CURGAME.",
    False,
)

"""
Names sub_2940E, called from UseAbilityOnTarget and
UnlockDoorCommand: a generic "targeting mode" wait loop. Sets the
cursor to a targeting icon (picture 0xF) via UpdateCursorForHeldItem,
then polls keyboard input until either ESC (cancel) or a valid click
on region table 0x5AC0 index 1 (the dungeon-viewport click region also
used by RunSellItemScreen's context).

-> WaitForTargetClick

Run via:
    .\run_ida_script.ps1 name_wait_for_target_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2940E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "WaitForTargetClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'WaitForTargetClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generic targeting-mode wait loop: sets a crosshair-style cursor "
    "(picture 0xF), polls input until ESC (cancel) or a valid click on "
    "the dungeon-viewport region (table 0x5AC0, index 1). Called from "
    "UseAbilityOnTarget and UnlockDoorCommand.",
    False,
)

"""
Names sub_1E4FA, called from RunAlchemyScreen: byte-for-byte identical
to the already-named ConfirmContainerInteraction (shows a yes/no
confirm prompt, message id 0x12, storing the result and current slot
selection for the caller) -- likely the same source routine compiled
into a different overlay segment, matching the DrawShadowedText/
DrawShadowedTextAlt duplication pattern found earlier this session.
-> ConfirmAlchemyInteraction

Run via:
    .\run_ida_script.ps1 name_confirm_alchemy_interaction.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E4FA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConfirmAlchemyInteraction", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConfirmAlchemyInteraction': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Byte-for-byte identical to ConfirmContainerInteraction (yes/no "
    "confirm prompt, message id 0x12, storing result + slot selection) "
    "-- likely duplicated into this overlay segment. Called from "
    "RunAlchemyScreen.",
    False,
)

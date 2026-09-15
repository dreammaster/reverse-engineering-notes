"""
Traced sub_1CBF3, another UseItem dispatch branch (UseItem+0x1A1,
distinct from UseKeyItem's UseItem+0x1B1) -- near-identical body to
UseKeyItem (same LoadLockState(ax=es:[si+0x10]) call, same
ShowResourceDepletedOverlay/word_36C7F save-restore dance, same
FinishItemUse tail), but missing UseKeyItem's word_328C6 bit 0x20
bracketing and its final ClearStatusPanelIfDirty call -- reads as a
lighter "check/preview this key" variant rather than the full "use it"
action.

-> CheckKeyItem

Run via:
    .\run_ida_script.ps1 name_check_key_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CBF3
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckKeyItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckKeyItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem dispatch branch for key items, near-identical to "
    "UseKeyItem (same LoadLockState(ax=es:[si+0x10]) call) but "
    "missing its word_328C6 bit 0x20 bracketing and final "
    "ClearStatusPanelIfDirty -- reads as a lighter check/preview "
    "variant rather than the full 'use this key' action.",
    False,
)

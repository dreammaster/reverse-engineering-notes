"""
Names sub_1AA06, called 6 times incl. from HandleIconBarItemExpiry and
sub_18C79: the exact ADD-side mirror of RemoveMultiStatEffect (walks
the loaded item's multi-stat-effect table, word_2E54A, adding each
entry's amount to the matching party field, capped at 0x3E7 this time
instead of floored at 0), finishing with the same sub_1AA9B +
UpdatePartyAverageStatTiers sequence. Distinct from the higher-level,
already-named ApplyMultiStatEffect (the "M" command handler, which
adds target confirmation, an incapacitation check, and a full redraw
sequence around similar core logic) -- this is the lower-level,
item-record-driven primitive. -> ApplyMultiStatEffectForItem

Run via:
    .\run_ida_script.ps1 name_apply_multi_stat_for_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AA06
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyMultiStatEffectForItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyMultiStatEffectForItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "ADD-side mirror of RemoveMultiStatEffect: walks the loaded item's "
    "multi-stat-effect table (word_2E54A), adding each entry's amount "
    "to the matching party field (capped at 0x3E7), then "
    "sub_1AA9B+UpdatePartyAverageStatTiers. Distinct from the "
    "higher-level ApplyMultiStatEffect command handler. Called from "
    "HandleIconBarItemExpiry and sub_18C79, among others.",
    False,
)

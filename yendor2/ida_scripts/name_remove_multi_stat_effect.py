"""
Names sub_1AC2F, called from PickUpItemFromSlot and sub_1819B: the
removal counterpart to the already-named ApplyMultiStatEffect. Loads
the item's catalog record and walks its multi-stat-effect table
(word_2E54A, up to 4 entries of [field-offset, amount]), subtracting
each entry's amount from the matching party-record field (offset
`+word_328D4`) -- fields with offset >= 0x32 are floor-clamped at 0
(uncapped subtraction otherwise). Finishes with sub_1AA9B and
UpdatePartyAverageStatTiers (matching ApplyMultiStatEffect's own
finish sequence). -> RemoveMultiStatEffect

Run via:
    .\run_ida_script.ps1 name_remove_multi_stat_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AC2F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RemoveMultiStatEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RemoveMultiStatEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Removal counterpart to ApplyMultiStatEffect: walks the item's "
    "multi-stat-effect table (word_2E54A) subtracting each entry's "
    "amount from the matching party-record field, floor-clamped at 0 "
    "for offset>=0x32 fields. Called from PickUpItemFromSlot and "
    "sub_1819B.",
    False,
)

"""
Round 4 continued: ExamineTarget. Confirmed same function (all of
yendor2's original calls -- TravelToDestination,
ShowAbilityDescriptionColumn x2, RestoreCursorBackgroundIfDirty,
UpdateCursorForHeldItem, DrawMouseCursorAlt -- are still present in the
same order), with a substantial new item-requirement gate prepended:
TestGlobalFlag, IsItemRangeAvailable, ClearGlobalFlag,
LoadItemCatalogRecord. Plausibly a new "you need item X to examine
this" check. See docs23/engine-diffs.md.

Run via:
    .\run_ida_script.ps1 apply_round4b.py
"""
import idc
import ida_name

ea = 0x2DA90
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ExamineTarget", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ExamineTarget': {'ok' if ok else 'FAILED'}")

"""
Traced sub_1CA64, called from UseItem's own fallback path (when the
item record's es:[si+0xE] bit 0x800 is set, distinct from the 4 main
type-handler branches). It re-loads the item's own type flags
(es:[si+0x10]) into word_2E410 and re-dispatches on the SAME bit
values UseItem's outer dispatch and the other handlers use (0x8000/
0x4000/0x400/0x800), but only to show a target-status preview -- it
calls ClassifyPartyMemberCondition, sub_1B7DD,
CheckPartyMemberItemFlagAndClearPanel, or CheckPartyMemberItemFlag
depending on which bit is set, with no resource cost or stat change
applied. Reads as "preview this item's target-status effect without
actually using it" -- consistent with the earlier finding that those
4 classifier functions build an item-target-selection status display.

A separate path (word_2E410 bits 0x3000, matching UseAbilityScroll's
selector) instead finishes the item use and shows a different kind of
result (an untraced pair, sub_25CFA/sub_2909C, when bit 0x1000 is also
set) -- not fully resolved, but distinct enough from the preview path
to note.

-> ShowItemUsagePreview

Run via:
    .\run_ida_script.ps1 name_show_item_usage_preview.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CA64
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowItemUsagePreview", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowItemUsagePreview': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem's fallback branch (item record es:[si+0xE] bit 0x800). "
    "Re-loads the item's own type flags (es:[si+0x10]) into "
    "word_2E410 and re-dispatches on the same bits the main type "
    "handlers use, but only to call a target-status classifier "
    "(ClassifyPartyMemberCondition / sub_1B7DD / "
    "CheckPartyMemberItemFlagAndClearPanel / CheckPartyMemberItemFlag) "
    "-- no cost or stat change applied. Reads as a preview of the "
    "item's target-status effect. A separate path (bits 0x3000, "
    "matching UseAbilityScroll's selector) instead finishes the use "
    "and shows a different result via an untraced pair "
    "(sub_25CFA/sub_2909C).",
    False,
)

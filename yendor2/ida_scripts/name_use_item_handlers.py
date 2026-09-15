"""
Traced one of UseItem's type-specific handlers, sub_1BEA1 (selected by
word_2E410 bit 0x400), plus two small helpers shared by it and the
type-0x800 handler (sub_1BBED, partially traced two rounds ago).

sub_1B702 -> SelectItemUseRecord: si = (word_2E550-1)*0x3A, es =
word_2E54C (LoadItemData's buffer segment). Sets es:si = word_2E54E,
the "current" 58-byte sub-record within the loaded item data
(word_2E550 is a 1-based sub-record index) -- confirms LoadItemData's
buffer holds a LIST of 58-byte item-use records, not a single blob.

sub_1B6DE -> FinishItemUse: common cleanup called at the end of every
branch in both sub_1BEA1 and sub_1BBED (sub_1B8AB, sub_1BA96, a
conditional redraw, sub_1B8EE) -- post-item-use redraw/cleanup step.

sub_1BEA1 -> UseItemType_400: sub-dispatches on the current item-use
record's own es:[si+0x10] flags (1, 4, 2, 0x200). The 0x2 branch is
the clearest: checks CompareBCD4(0x94B3 vs a cost at 0x512A), shows a
"not enough" message if short, otherwise SubBCD4 to pay the cost, runs
several untraced follow-up calls, then sets a per-character flag via
SetRecordFlag_10C using the item catalog record's own +0x1A field as
the flag index -- the same "consume a BCD material, then mark a
personal one-time-event flag" pattern already seen in sub_1BBED's
type-2 branch (two rounds ago). The 0x200 branch instead builds a
message string (FormatNumber + StrCat chain) without any BCD cost --
plausibly a "read/examine" flavor-text path rather than a consuming
one. Named on its dispatch-selector bit rather than a guessed item
category, consistent with the _10C/_CA convention from last round,
since sub_1BBED (the 0x800-selected sibling) hasn't been fully
resolved either.

Run via:
    .\run_ida_script.ps1 name_use_item_handlers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1B702: "SelectItemUseRecord",
    0x1B6DE: "FinishItemUse",
    0x1BEA1: "UseItemType_400",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1B702,
    "SelectItemUseRecord: es:si = word_2E54E = the (word_2E550)th "
    "58-byte sub-record within LoadItemData's buffer (es=word_2E54C). "
    "Confirms the loaded item block is a list of use-records, not a "
    "single blob.",
    False,
)
ida_bytes.set_cmt(
    0x1B6DE,
    "FinishItemUse: common post-item-use cleanup/redraw, called at "
    "the end of every branch in UseItemType_400 and sub_1BBED "
    "(the 0x800-selected sibling handler).",
    False,
)
ida_bytes.set_cmt(
    0x1BEA1,
    "One of UseItem's item-type handlers (selected by word_2E410 bit "
    "0x400). Sub-dispatches on the current SelectItemUseRecord's own "
    "es:[si+0x10] flags. Bit 2: pays a BCD material cost (0x94B3 vs a "
    "threshold at 0x512A, 'not enough' message if short) then sets a "
    "per-character flag via SetRecordFlag_10C using the item's own "
    "+0x1A field as the index -- same pattern as sub_1BBED's type-2 "
    "branch. Bit 0x200: builds a message string instead, no BCD cost "
    "-- plausibly a non-consuming 'read/examine' path.",
    False,
)

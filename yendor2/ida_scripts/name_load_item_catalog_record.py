"""
Names sub_12554 -- the single most pervasively-used lookup function
this session (dozens of callers across nearly every subsystem
explored: DrawListEntryLabel, ApplyMultiStatEffect, HandleGameCommand's
own entry, TickAilmentDuration/FindItemInInventoryRange's container
checks, etc.).

sub_12554(ax=item id): maps in EMS item-catalog pages, copies the
item's main 58-byte record (0x1D words -- matches
SelectItemUseRecord's record size) into a scratch buffer at 0xB50.
Then, if the record's own [+2] field is nonzero, also sets up
word_2E54A (the multi-stat-effect table ApplyMultiStatEffect walks)
from an 8-word/16-byte sub-block. Also sets up word_2E548 -- THE
"current target" record pointer read throughout the codebase
(RunConversation, HandleGameCommand's fallback dispatch,
ShowLockStatus, etc.) -- based on a separate flag test on the item's
own [+0xC] field. So looking up an item's catalog record is also how
the game establishes "the current target" context for whatever
happens next.

-> LoadItemCatalogRecord

Run via:
    .\run_ida_script.ps1 name_load_item_catalog_record.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12554
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadItemCatalogRecord", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadItemCatalogRecord': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "LoadItemCatalogRecord(ax=item id): maps in EMS item-catalog "
    "pages, copies the item's 58-byte record into a scratch buffer "
    "(0xB50). If [+2] is nonzero, also loads word_2E54A (the "
    "multi-stat-effect table ApplyMultiStatEffect walks) from an "
    "8-word sub-block. Also sets up word_2E548 -- the 'current "
    "target' pointer read throughout the codebase -- based on a "
    "flag test on [+0xC]. The single most pervasively-used item "
    "lookup in the executable.",
    False,
)

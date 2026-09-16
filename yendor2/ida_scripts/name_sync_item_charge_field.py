"""
Names sub_2778D, called 3 times from unnamed sub_274B4 (the large item-
use/consumption handler that already hosts SwapItemMultiStatEffect and
SyncAlternateBagsToSave), each time with a different (ax, dx) pair
(word_3296C/word_32972/word_3296E as ax; word_3297C/0/0 as dx).

Before the 3 calls, the caller configures a resource-stub helper
(ax=0xAFA8 -- a heavily-reused shared scratch buffer, e.g. also used
for conversation-topic text -- bx=8FFB/CURGAME) and stamps a marker
byte at [0xAFA8+6]=0x22.

This function then: FileEntry_Read's (bx=8FFB/CURGAME, errorCode=0xA)
the just-configured record, applies the *same* 3-way
category-dependent charge/transfer/swap logic the caller applies to
its own in-memory copy (word_328C8 bits 0x8000/0x4000/0x2000: transfer
a secondary count into primary; decrement a shared charge counter at
word_2E548, zeroing on depletion; or call SwapItemMultiStatEffect),
against the field at [0xAFA8+dx] -- then FileEntry_Write's the record
back to CURGAME, unless the item is the "transfer" category (bit
0x8000) *and* dx==0, in which case it instead subtracts the staged
amount (word_3293E) from a generic reused scratch variable
(word_38808 -- documented elsewhere as unrelated-purpose scratch, not
a specific resource pool) and skips the write.

In short: persists one item-record charge/quantity field to CURGAME,
mirroring the caller's in-memory charge-handling logic exactly, for a
shared on-disk record buffer. The exact identity of the 3 fields
(word_3296C/32972/3296E) and of word_38808's role in this specific
context are not confirmed. -> SyncItemChargeFieldToCurgame

Run via:
    .\run_ida_script.ps1 name_sync_item_charge_field.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2778D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SyncItemChargeFieldToCurgame", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SyncItemChargeFieldToCurgame': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Reads (FileEntry bx=8FFB/CURGAME, errorCode=0xA) the shared "
    "0xAFA8 scratch record the caller just configured, applies the "
    "same category-dependent charge/transfer/swap logic sub_274B4 "
    "applies to its in-memory copy to the field at [0xAFA8+dx], then "
    "writes it back -- except for the 'transfer' category with dx==0, "
    "where it instead subtracts the staged amount (word_3293E) from "
    "scratch var word_38808 and skips the write. Called 3x from "
    "sub_274B4 with different (ax,dx) field selectors.",
    False,
)

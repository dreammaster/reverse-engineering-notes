"""
Traced sub_2A914 -- the very first thing HandleGameCommand checks
(gated on word_2E54A, a per-target table pointer populated from the
target's own catalog lookup via sub_12554 at HandleGameCommand's top).

Walks up to 4 entries in the table at word_2E54A: each is an
(offset, amount) pair. For each with a nonzero offset, if the current
party member (word_328D4) already has a nonzero value at that field
offset, adds the amount to it and calls sub_2A982 (not traced). Reads
as "apply this item's stat bonuses" -- a target/item with a populated
word_2E54A table can bump up to 4 different party-member fields at
once (only for fields that already hold a value, i.e. real stats, not
uninitialized ones) -- matches an equip-bonus or multi-effect
consumable mechanic. Falls back to a simple redraw sequence if the
table pointer is null or the party member is invalid.

-> ApplyMultiStatEffect

Run via:
    .\run_ida_script.ps1 name_apply_multistat_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2A914
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyMultiStatEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyMultiStatEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "First thing HandleGameCommand checks: if the current target has "
    "a populated word_2E54A table (up to 4 (offset,amount) pairs, "
    "from the target's own catalog lookup), applies each nonzero "
    "entry to the party member's matching field (only if that field "
    "already holds a value) via sub_2A982. Reads as an equip-bonus "
    "or multi-effect consumable mechanic. Falls back to a simple "
    "redraw sequence if the table is null or the member is invalid.",
    False,
)

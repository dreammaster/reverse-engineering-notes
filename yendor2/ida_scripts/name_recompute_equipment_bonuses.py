"""
Names sub_1B30C, called from sub_18C79, sub_1AA9B, and the still-
untraced item-use dispatcher sub_274B4 (right after
ApplyMultiStatEffectForItem) -- a full recomputation of the current
party member's (si=word_328D4) equipment-derived stat bonuses.

First resets two 5-word "derived bonus" blocks from their base
values: [si+0x88..0x90] <- [si+0x72..0x7A], and [si+0x48..0x50] <-
[si+0x32..0x3A] (both pairs 0x16 apart -- a different pairing scheme
than the confirmed +0x40 base/derived convention, specific to
equipment bonuses).

Then, for each of the character's equipped item slots -- the main
weapon (+0x13A), a second slot (+0x142, whose catalog-flag bits
0x4000/0x2000/0x1000 select which of 3 bonus-field pairs its value
adds into), a 3-entry array (+0x146, stride 4), and a 5-entry array
(+0x152, stride 2) -- loads the item's catalog record
(LoadItemCatalogRecord) and adds its stat bonus ([bx], the record's
own leading field) into the running totals at [si+0x48]/[si+0x88] (and
siblings). The second slot's item also conditionally sets a UI flag
([si+0x15C] bit 0x20, cleared unconditionally at the top of the
function first) when its catalog flags bit 0 is set.

In short: recomputes all equipment-derived stat bonuses from scratch
whenever gear changes. -> RecomputeEquipmentStatBonuses

Run via:
    .\run_ida_script.ps1 name_recompute_equipment_bonuses.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B30C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RecomputeEquipmentStatBonuses", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RecomputeEquipmentStatBonuses': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resets [si+0x48..0x50]/[si+0x88..0x90] from their base values "
    "([si+0x32..0x3A]/[si+0x72..0x7A]), then adds each equipped "
    "item's catalog stat bonus (weapon +0x13A, slot +0x142, 3-array "
    "+0x146, 5-array +0x152) via LoadItemCatalogRecord. Also sets/"
    "clears [si+0x15C] bit 0x20 from the +0x142 item's catalog flags. "
    "A full equipment-derived stat recompute. Called from sub_18C79, "
    "sub_1AA9B, and sub_274B4.",
    False,
)

"""
Names sub_274B4, called from 21 sites (CheckAndPaySpecialItemCost,
HandleSearchCommand's failed-trap-search backfire, RestPartyAndAdvanceClock,
and many more in the item-ability-effect region) -- the shared
"spend one use of an item-based resource" engine.

Confirmed via CheckAndPaySpecialItemCost: called right after
IsItemRangeAvailable locates a qualifying inventory item (sets
word_32974 to reference it), to actually consume it as payment.
Relies entirely on caller-configured globals rather than explicit
register parameters:

- word_32974: the item/event code to load (LoadItemCatalogRecord).
- word_3297A: a "charge record" pointer (2 fields, [+0]/[+2]) or 0.
  If 0, falls back to a party-record-relative field
  (word_32978 + word_3297C) instead -- word_3297C reuses the
  confirmed equipment-slot offsets 0x13A/0x142/0x146.
- word_328C8 bits 0x8000/0x4000/0x2000 select the consumption mode
  (checked identically in both the word_3297A and the fallback path):
  - 0x8000: transfers the charge pair's [+2] into [+0] and clears
    [+2], then zeroes the matching equipped-item wear counter
    ([+0xBE]/[+0xC0]/[+0xC2] by slot offset -- the same counters
    TickEquippedItemDurability increments) -- a "full recharge,
    reset wear" mode.
  - 0x4000: zeroes both charge fields outright -- full discard.
  - 0x2000: calls SwapItemMultiStatEffect (swaps the item's applied
    stat effect for an alternate) before discarding.
  - default: if the item's word_2E548 category flag bit 0x1 isn't
    set, discards outright; otherwise decrements the [+2] charge
    counter, auto-discarding once it hits 0 -- a normal "use one
    charge" tick.
  A separate branch (word_3296C != 0) instead syncs 3 related charge
  fields straight to the CURGAME file via 3x
  SyncItemChargeFieldToCurgame calls.
- Unless the transfer-mode bit (0x8000) is set, subtracts the
  consumed item's staged weight (word_3293E, from the catalog
  record's [+0xA]) from the owning party member's carried weight
  ([+0x118], the confirmed carry-capacity field).
- Identifies which of the 4 party slots owns the change
  (word_32976 against g_partySlotAssignment/word_36E4D/word_36E4F/
  word_36E51), and if that slot's redraw flag (word_328C6 bit
  0x4000/0x2000/0x1000/0x800) is set, syncs its alternate bags
  (SyncAlternateBagsToSave), redraws its portrait
  (DrawPartyMemberPortrait), and — if any consumption-mode bit fired
  — reapplies the held item's stat effect (ApplyMultiStatEffectForItem)
  and recomputes equipment bonuses (RecomputeEquipmentStatBonuses).

-> ConsumeItemChargeResource

Run via:
    .\run_ida_script.ps1 name_consume_item_charge_resource.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x274B4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConsumeItemChargeResource", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConsumeItemChargeResource': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shared 'spend one use of an item-based resource' engine, driven "
    "entirely by caller-configured globals (word_32974 item code, "
    "word_3297A charge-record pointer or 0 for a party-record field "
    "fallback, word_328C8 bits 0x8000/0x4000/0x2000 selecting the "
    "consumption mode: recharge+reset-wear / full discard / swap-"
    "effect-then-discard / default decrement-with-auto-discard). "
    "Deducts the item's weight, then syncs bags, redraws the owning "
    "party member's portrait, and reapplies stat effects/equipment "
    "bonuses. Called from CheckAndPaySpecialItemCost, "
    "HandleSearchCommand, and ~19 more sites.",
    False,
)

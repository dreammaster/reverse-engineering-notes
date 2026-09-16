"""
Names sub_1819B, called from ApplyEffectAndDrawIconBar: a significant
find -- handles an icon-bar item's effect expiring. si is an icon-bar
slot (the confirmed 0xC50+slot*0x14 layout: [+0xC]=character record,
[+0xE]=slot-address delta, [+0x10]=item id, [+0x12]=a word_2E548-
derived field).

Temporarily switches the "current character" (word_328D4) to the
slot's owner, calls RemoveMultiStatEffect to strip the expiring item's
stat bonuses, then branches on a flag ([di+0xA] bit 0x200):
- Clear: replaces the inventory slot's item with a new one ([si+0x12]),
  loads its catalog record, and applies its effect via sub_1AA06
  (plausibly ApplyMultiStatEffect's sibling/twin for the new item) --
  the item "transforms" into something else.
- Set: clears the slot's item id entirely and subtracts the expiring
  item's weight from the confirmed weight counter (+0x118) -- the item
  is destroyed/consumed outright.

Finishes with sub_1AA9B + UpdatePartyAverageStatTiers, matching
RemoveMultiStatEffect/ApplyMultiStatEffect's own finish sequence.
-> HandleIconBarItemExpiry

Run via:
    .\run_ida_script.ps1 name_handle_item_expiry.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1819B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleIconBarItemExpiry", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleIconBarItemExpiry': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Handles an icon-bar item's effect expiring: removes its stat "
    "bonuses (RemoveMultiStatEffect), then either replaces the "
    "inventory slot with a new item (applying its effect via "
    "sub_1AA06) or clears the slot and subtracts the item's weight "
    "(+0x118) -- item transforms or is destroyed. Called from "
    "ApplyEffectAndDrawIconBar.",
    False,
)

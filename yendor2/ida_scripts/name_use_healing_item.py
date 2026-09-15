"""
Traced sub_1BF94, UseItem's handler for word_2E410 bit 0x8000 (the
first bit UseItem tests). Unlike the other type handlers named so far
(UseItemType_400, sub_1BBED), this one has concrete, unambiguous
healing/cure behavior, so it gets a real name instead of an offset-
suffixed placeholder:

Bit 2 of the current item-use record (SelectItemUseRecord) is the
"consume a material and heal" path: pays a BCD cost (CompareBCD4/
SubBCD4 against 0x94B3 vs a threshold at 0x512A, same "not enough"
message pattern as the other handlers), then reads word_3298E (a
separate cure-flags word, apparently sourced from the item's own data)
to decide which ailments/effects to cure on the current party member
(word_328D4): bit 0x2000 clears status bit 6 and sets HP to 2
(plausibly "wake from unconsciousness"); bit 0x4000 clears status bit
7; bit 0x8000 sets HP to max; bit 0x1000 also sets HP to max and
additionally clears status bits 0-5. It then reuses the trap/status-
effect icon-bar system (PrepareTrapEffectSlots(ax=3) +
ApplyEffectAndDrawIconBar) to show a "healed" icon on the target, and
refreshes their status via ClassifyPartyMemberCondition + sub_22445
(the party-portrait redraw from 3 rounds ago).

The other branch (loc_1C0D7, reached when none of bits 1/4/2/0x1000
match) instead reads word_2E40C's ClassifyPartyMemberCondition tier
bits (0x4000/0x8000/0x2000) to pick a message-index offset and shows a
condition-appropriate message via sub_1B96F -- confirms
ClassifyPartyMemberCondition's tiers really do drive user-facing
status text, not just internal bookkeeping.

-> UseHealingItem

Run via:
    .\run_ida_script.ps1 name_use_healing_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1BF94
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseHealingItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseHealingItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem's handler for word_2E410 bit 0x8000. Its bit-2 branch is "
    "unambiguous healing/cure: pays a BCD material cost (0x94B3 vs "
    "threshold 0x512A), then applies cure effects per word_3298E "
    "flags (0x2000: clear status bit 6 + HP=2 -- plausibly wake from "
    "unconsciousness; 0x4000: clear status bit 7; 0x8000: HP=max; "
    "0x1000: HP=max + clear status bits 0-5) to word_328D4, draws a "
    "'healed' icon via the trap-effect icon-bar system "
    "(PrepareTrapEffectSlots effect id 3 + ApplyEffectAndDrawIconBar), "
    "and refreshes the target's status (ClassifyPartyMemberCondition, "
    "sub_22445). A separate branch (reached when no low item-record "
    "bits match) picks a status message by ClassifyPartyMemberCondition's "
    "word_2E40C tier bits -- confirms those tiers drive user-facing "
    "text.",
    False,
)

"""
CORRECTION: identifies DS:0x94B3 (linear 0x36D13) as g_partyGold, not
a generic "material counter" as previously framed, and renames its two
known consumer functions accordingly.

Evidence:
- ShowMaterialCounterHud draws a label next to 0x94B3's BCD4 value.
  Dumping msg 0x7FC4 (the label) shows it is a literal single "$"
  character -- the currency symbol -- immediately followed in the same
  message bank by "SPACEBAR TO / SELL ITEM OR / ESC TO UNDO" and then
  "I HAVE NO NEED FOR THAT TYPE OF ITEM." -- the exact prompt/rejection
  pair used by sub_19264 (named TryConvertItemToMaterial last round).
  So sub_19264 is a sell-item-for-gold action, not a generic material
  conversion.
- sub_18FDA (an "enhance item" action: CompareBCD4/SubBCD4 against
  0x94B3, then advances to the next item in the catalog) shows
  "YOU DON'T HAVE ENOUGH GOLD!" (msg 0x8376) when the CompareBCD4
  check fails -- direct textual proof 0x94B3 is spent as gold there.

This does not change 0x94B7/0x94BB, which remain confirmed as the
NUORE/MAGIC ORE alchemy counters. It also doesn't necessarily change
ApplyEffectCost's use of the same 3-counter family for trap/status
effect costs -- a trap stealing party gold is plausible and consistent.

Renames:
  0x36D13 (data)  -> g_partyGold
  0x19264 (func)  TryConvertItemToMaterial -> TrySellItemForGold
  0x18FDA (func)  sub_18FDA -> TryEnhanceItemForGold

Run via:
    .\run_ida_script.ps1 fix_gold_counter_identity.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x36D13: "g_partyGold",
    0x19264: "TrySellItemForGold",
    0x18FDA: "TryEnhanceItemForGold",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x36D13,
    "Party gold (packed-BCD4, most-significant-digit-first). HUD label "
    "is a literal '$' (msg 0x7FC4, via ShowMaterialCounterHud). Spent "
    "by TryEnhanceItemForGold (per-tier cost table at DS:0xCB2), "
    "credited by TrySellItemForGold (sells a held item of a matching "
    "type), and also touched by ApplyEffectCost's trap/status-effect "
    "cost dispatch alongside the two ore counters (0x94B7/0x94BB).",
    False,
)

ida_bytes.set_cmt(
    0x19264,
    "Space-bar 'sell item' action (sub_1869D main loop, word_328C6 bit "
    "0x10) while carrying an item: if the held item's type mask "
    "doesn't overlap the standing location's accepted-type mask, shows "
    "'I HAVE NO NEED FOR THAT TYPE OF ITEM.' (msg 0x7FF7). Otherwise "
    "sells the item, crediting its value (word_32920, via AddBCD4) to "
    "g_partyGold, then ShowMaterialCounterHud. Renamed from "
    "TryConvertItemToMaterial after confirming g_partyGold's identity "
    "(HUD label is a literal '$', and the 'SPACEBAR TO SELL ITEM OR "
    "ESC TO UNDO' prompt lives in the same message bank).",
    False,
)

ida_bytes.set_cmt(
    0x18FDA,
    "Space-bar 'enhance item' action (sub_1869D, sibling of "
    "TrySellItemForGold). Eligibility via sub_1B147 (a level/stat "
    "range check against table 0xBCE); on failure, 'I CAN NOT ENHANCE "
    "THAT' (msg 0x815A). Else CompareBCD4(g_partyGold, [table 0xCB2]) "
    "-- on insufficient gold, 'YOU DON'T HAVE ENOUGH GOLD!' (msg "
    "0x8376, via sub_190AF); else SubBCD4(g_partyGold -= [0xCB2]), "
    "advances the item to the next catalog entry (word_32974+1) and "
    "reloads it as the enhanced result.",
    False,
)

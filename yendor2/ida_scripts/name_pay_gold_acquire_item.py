"""
Names sub_17A8D, called from sub_17032 (a shop-catalog click handler,
itself reached from sub_1869D's main input loop on a mouse-click hit
against region table 0x5AC0, gated by word_328C6 bit 0x200 -- a 4th
"shop mode" bit alongside TrySellItemForGold/TryEnhanceItemForGold/
TryRepairItemForGold's 0x10/8/4).

Mechanism: CompareBCD4(g_partyGold, [0xB30]) -- if gold < price,
clears word_31948 and bails (can't afford). Else SubBCD4(g_partyGold
-= [0xB30]); if gold == price exactly (a total-drain special case),
also clears word_36D6D and shows ShowResourceDepletedOverlay before
paying. Then stages the acquired item's fields (word_31946/3194A from
word_2E546, word_3194C from si+2) the same way
TrySellItemForGold/TryEnhanceItemForGold/TryRepairItemForGold stage
their results, refreshes drawing (sub_23874), and calls
ShowMaterialCounterHud/DrawMouseCursor.

The core "pay gold and receive the item" step of a shop purchase.
-> PayGoldAndAcquireItem

Run via:
    .\run_ida_script.ps1 name_pay_gold_acquire_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17A8D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PayGoldAndAcquireItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PayGoldAndAcquireItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Core 'pay and receive' step of a shop purchase: CompareBCD4/"
    "SubBCD4(g_partyGold, [0xB30]) -- bails if unaffordable, shows "
    "ShowResourceDepletedOverlay on an exact-drain special case -- "
    "then stages the acquired item (word_31946/3194A/3194C) the same "
    "way TrySellItemForGold/TryEnhanceItemForGold/TryRepairItemForGold "
    "stage theirs. Called from sub_17032, a shop-catalog click handler "
    "(main input loop, word_328C6 bit 0x200).",
    False,
)

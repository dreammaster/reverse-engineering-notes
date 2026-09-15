"""
Names sub_17B09, called from sub_17032 (the shop-catalog click
handler whose "buy" branch is PayGoldAndAcquireItem) -- a sibling
branch that credits gold instead of spending it: AddBCD4(g_partyGold,
[0xB30]) adds the price back into the party's gold, shows
ShowResourceDepletedOverlay once (word_36D6D latch), clears the
held/staged item (word_31946/31948/3194A/3194C), refreshes drawing and
ShowMaterialCounterHud.

Reads as a "sell this catalog item back" action -- the mouse-click
counterpart to TrySellItemForGold, reached via a different sub_17032
branch than the click-to-buy path.

-> SellClickedCatalogItem

Run via:
    .\run_ida_script.ps1 name_sell_clicked_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17B09
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SellClickedCatalogItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SellClickedCatalogItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Credits gold (AddBCD4(g_partyGold, [0xB30])) instead of spending "
    "it, clears the held/staged item, refreshes the material/gold HUD "
    "-- a 'sell this catalog item back' action, the click counterpart "
    "to TrySellItemForGold. Called from sub_17032.",
    False,
)

"""
Names sub_1B245, called from UseItem when the used item's [+0xE] flags
have bit 0x4000 set (a sibling branch to a "BUY "-named item check
leading to sub_1B2BD). Sets word_328C6 bit 0x10 -- the exact bit that
gates TrySellItemForGold in sub_1869D's main input loop -- draws the
resource-depleted overlay and material HUD, then calls sub_1869D
itself (the main input loop) so the player can interactively sell
items via Space, clears word_328CA's "accepted" flag and the staged
item id, restores state, and rebuilds/redraws the minimap on exit.

The entry point for the interactive "sell item" screen/station.
-> RunSellItemScreen

Run via:
    .\run_ida_script.ps1 name_run_sell_item_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B245
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunSellItemScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunSellItemScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Entry point for the interactive sell-item screen, reached from "
    "UseItem when the used item's [+0xE] flags have bit 0x4000 set. "
    "Sets word_328C6 bit 0x10 (gates TrySellItemForGold in the main "
    "input loop sub_1869D), shows the resource-depleted overlay and "
    "material/gold HUD, runs sub_1869D so Space sells items, then on "
    "exit clears state and rebuilds/redraws the minimap.",
    False,
)

"""
Names sub_21C79, called from sub_17032 (shop buy handler) right after
LoadItemCatalogRecord, and from sub_1869D (the trade/inventory screen
dispatcher) as the default branch after LoadItemCatalogRecord when no
barter-pricing preview is needed (the alternate branch instead calls
ComputeBarterPricingPreview + sub_219FA).

Clears the status panel if dirty, sets word_328C4 bit 0x100, targets
the main video buffer, positions text at (0xF0, 0x60) with a fixed
bg/fg color scheme, then calls DrawStringColumn with bx = word_2E546
(the current item-catalog-record pointer, confirmed elsewhere: clue
book icon source, barter/repair eligibility field source) + 0x13,
cx=3 -- a 3-line text field from the item record, almost certainly
the item's description text. Finishes with ShowMaterialCounterHud and
DrawMouseCursor. Reads as: redraw the current item's description text
alongside the material-cost HUD, the standard post-item-load refresh
in the shop/trade screens. -> RedrawItemDescriptionAndMaterials

Run via:
    .\run_ida_script.ps1 name_redraw_item_description_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21C79
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RedrawItemDescriptionAndMaterials", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RedrawItemDescriptionAndMaterials': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears status panel if dirty, sets word_328C4 bit 0x100, "
    "positions text at (0xF0,0x60), draws a 3-line text field from "
    "word_2E546+0x13 (current item record's description text) via "
    "DrawStringColumn, then ShowMaterialCounterHud + DrawMouseCursor. "
    "Standard post-LoadItemCatalogRecord refresh in shop/trade "
    "screens. Called from sub_17032 and sub_1869D.",
    False,
)

"""
Names sub_219FA, called from TryHandleCatalogSlotClick,
PayGoldAndAcquireItem, and HandleStatusPanelItemSlotClick (16 refs
total) -- the item-purchase/cost confirmation prompt.

Caches ax into word_32974 (item/event code) and bx into word_3293E
(a price adjustment). Unless a word_328C6 mode-flag group (bits
0x3C) is set, prompts the player to pick a caster/recipient party
member (ShowConfirmPrompt id 7), rejecting an incapacitated choice
([+0x1C] bits 0x1C40). Then redraws the item description
(RedrawItemDescriptionAndMaterials) and draws a labeled price line:
the item's base cost (word_2E546+0xA) plus word_3293E, formatted with
a decimal point via InsertDecimalPointFromEnd. Continues (not fully
traced) into further price/confirmation UI. -> ShowItemPurchaseConfirmPrompt

Run via:
    .\run_ida_script.ps1 name_show_item_purchase_confirm_prompt.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x219FA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowItemPurchaseConfirmPrompt", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowItemPurchaseConfirmPrompt': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Item-purchase/cost confirmation prompt: caches ax=item/event "
    "code, bx=price adjustment, prompts for a caster/recipient party "
    "member unless a mode-flag group is set, redraws the item "
    "description, and draws the formatted total price line. Called "
    "from TryHandleCatalogSlotClick, PayGoldAndAcquireItem, and "
    "HandleStatusPanelItemSlotClick.",
    False,
)

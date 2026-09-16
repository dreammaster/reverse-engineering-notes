"""
Names sub_17032, called from RunShopScreen and sub_1869D -- handles
a click on the shop's catalog item grid (hit-tested via
HitTestCatalogSlot).

If the player is already holding an item, defers to a different
branch (loc_171B8, not traced here). Otherwise, for an empty
hand: if word_328C6 bit 0x20 is set, buys the clicked item directly
via PayGoldAndAcquireItem. Else dispatches on the item's own [+0xC]
flag bits: bit 0x80 -> sell it for gold (TriggerSoundEvent + credits
g_partyGold via AddToBCDCounter); bit 0x40 / bit 0x20 -> other
branches (not traced here); default -> picks the item up into the
held-item state (word_31946 quantity, word_2E530 picture id,
word_3194A/3194C), clears the slot, and redraws the shop grid
(DrawShopItemSlotGrid) + item description
(RedrawItemDescriptionAndMaterials). -> HandleShopCatalogSlotClick

Run via:
    .\run_ida_script.ps1 name_handle_shop_catalog_slot_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17032
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleShopCatalogSlotClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleShopCatalogSlotClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Click handler for the shop's catalog item grid "
    "(HitTestCatalogSlot). With an empty hand: buys directly via "
    "PayGoldAndAcquireItem if word_328C6 bit 0x20 is set, else "
    "dispatches on the item's [+0xC] flags (0x80=sell for gold, "
    "0x40/0x20=other branches, default=pick up into held-item "
    "state). Called from RunShopScreen and sub_1869D.",
    False,
)

"""
Names sub_1728A, called twice from sub_17032 (the shop buy handler).

Clears a rectangular video-buffer region (35 rows x 36 words, the
shop's item-slot grid area), then loops 8 times over a position
table (0x63C8) and an item-id source table (0x558A): for each nonzero
item id, loads its catalog record via LoadItemCatalogRecord and draws
its icon ([+8] field, picture slot 8) at the paired position,
counting how many were drawn (word_3293E). If none were drawn (the
shop has no items to show), shows "EMPTY" (confirmed via string dump
at 0x7D26) and sets a word_328C6 flag bit. -> DrawShopItemSlotGrid

Run via:
    .\run_ida_script.ps1 name_draw_shop_item_slot_grid.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1728A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawShopItemSlotGrid", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawShopItemSlotGrid': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears the shop item-slot grid area, then draws up to 8 item "
    "icons (from a position table at 0x63C8 and an item-id table at "
    "0x558A) via LoadItemCatalogRecord + DrawPicture. Shows 'EMPTY' "
    "and sets a word_328C6 flag bit if no items were drawn. Called "
    "twice from sub_17032.",
    False,
)

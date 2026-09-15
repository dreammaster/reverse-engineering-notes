"""
Names sub_23874, called from `start` directly and throughout most of
this session's item-manipulation functions (TrySellItemForGold,
PayGoldAndAcquireItem, TryDropHeldItem, etc.) right after staging or
clearing a held item.

Sets word_31946 (the "carrying this item" icon/flag) from word_2E530
(the current item id), builds a cursor-sprite definition (a lookup
table at 0x782E+word_2E532, feeding word_36873/75/77/79/7B/7D) and
reads its data via FileEntry (bx=0x9011). If word_3195C bit 1 is set,
also restores the old cursor background and redraws
(RestoreCursorBackground + sub_23965).

Updates the mouse cursor to show the currently-held item's icon.

-> UpdateCursorForHeldItem

Run via:
    .\run_ida_script.ps1 name_update_cursor_held_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23874
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UpdateCursorForHeldItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UpdateCursorForHeldItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Updates the mouse cursor to show the currently-held item's icon "
    "(word_2E530 -> word_31946), rebuilding the cursor-sprite "
    "definition via FileEntry (bx=0x9011). Called throughout the "
    "item-manipulation functions after staging/clearing a held item.",
    False,
)

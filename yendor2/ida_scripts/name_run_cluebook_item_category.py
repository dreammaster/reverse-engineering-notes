"""
Names sub_13090, called from ShowClueBook (once): the interactive
per-entry loop for the F5 "INVENTORY ITEMS" clue-book category. Sets
word_328CC bit 0x40 (the nav bar's "d) LIST" hint gate), loads the
first entry id from a table at word_2E3EE, then loops: draw the entry
via ShowClueBookItemDetail + sub_13780, poll keyboard/mouse, hit-test
a region table (0x6976) to let the player click a specific
sub-icon/index within the panel (tracked in word_328FE), and advance
word_32974 to the corresponding entry -- until ESC (sub_14D26 sets
word_2E40A).

Note: sub_13119, called from two other ShowClueBook sites and also
calling ShowClueBookItemDetail, adds extra dispatches for item-id
ranges that overlap CastSpell's (0x12-0x1D) and RestCharacter's
(0x36-0x46) selector ranges -- plausibly spell-scroll/ability item
detail overlays. Left unnamed; not fully traced.

-> RunClueBookItemCategory

Run via:
    .\run_ida_script.ps1 name_run_cluebook_item_category.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13090
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunClueBookItemCategory", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunClueBookItemCategory': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "F5 'INVENTORY ITEMS' clue-book category loop (called once from "
    "ShowClueBook). Draws the current entry via ShowClueBookItemDetail, "
    "polls input, hit-tests a region table (0x6976) so the player can "
    "click a sub-icon to jump to a specific entry (word_328FE tracks "
    "the selection), loops until ESC.",
    False,
)

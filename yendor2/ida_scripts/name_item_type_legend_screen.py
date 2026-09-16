"""
Names sub_13463 and its two row-drawing helpers sub_135E8/sub_13630,
called once from WaitForKeypress (itself called from ShowClueBook) --
a clue book sub-page.

sub_13463: draws a message box + DrawClueBookNavBar, then 6 rows,
each an icon picture plus a label row via one of the two helpers
below. Confirmed via string dump: the 6 row labels are "POTIONS",
"SCROLLS", "WANDS", "VIALS", "PARCHMENTS", "RODS" -- the game's 6
consumable item categories -- each row reading "<TYPE> PERMANENTLY
ADD <n> TO AN ATTRIBUTE" or "...TO A SKILL" (the digit is byte_2E400,
here repurposed to hold a literal display character rather than its
usual last-keypress meaning). Directly explains
UseAttributeBoostItem/UseExperienceBoostItem-style consumables: this
is the clue book's in-game legend for what each item category
permanently boosts. -> ShowConsumableItemTypeLegend

sub_135E8: draws one row ending in "...TO AN ATTRIBUTE" (suffix
strings "PERMANENTLY ADD" + digit + "TO AN" + "ATTRIBUTE").
-> DrawItemTypeLegendAttributeRow

sub_13630: draws one row ending in "...TO A SKILL" (suffix strings
"PERMANENTLY ADD" + digit + "TO A" + "SKILL").
-> DrawItemTypeLegendSkillRow

Run via:
    .\run_ida_script.ps1 name_item_type_legend_screen.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x13463, "ShowConsumableItemTypeLegend",
     "Clue book sub-page: draws a message box + DrawClueBookNavBar, "
     "then 6 rows -- POTIONS/SCROLLS/WANDS/VIALS/PARCHMENTS/RODS "
     "(confirmed via string dump), each explaining that the item "
     "type 'PERMANENTLY ADD<n> TO AN ATTRIBUTE' or '...TO A SKILL' "
     "via DrawItemTypeLegendAttributeRow/DrawItemTypeLegendSkillRow. "
     "Called once from WaitForKeypress (itself called from "
     "ShowClueBook)."),
    (0x135E8, "DrawItemTypeLegendAttributeRow",
     "Draws one item-type-legend row ending '...TO AN ATTRIBUTE' "
     "(a digit char via byte_2E400, repurposed here as a display "
     "character). Called from ShowConsumableItemTypeLegend."),
    (0x13630, "DrawItemTypeLegendSkillRow",
     "Draws one item-type-legend row ending '...TO A SKILL' (a digit "
     "char via byte_2E400, repurposed here as a display character). "
     "Called from ShowConsumableItemTypeLegend."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)

"""
Names sub_13678, a clue-book entry detail screen (calls DrawMessageBox
+ DrawClueBookNavBar, then draws an icon for the current entry
(word_2E546) plus two labeled stat fields). Dumped the field labels:
"BASE VALUE:" and "WEIGHT:" -- this is the F5 "INVENTORY ITEMS"
category's item detail screen (icon, base value, weight), one of the
categories listed by ShowClueBookHelpScreen.

-> ShowClueBookItemDetail

Run via:
    .\run_ida_script.ps1 name_cluebook_item_detail.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13678
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowClueBookItemDetail", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowClueBookItemDetail': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clue book 'F5 INVENTORY ITEMS' entry detail screen: message box + "
    "DrawClueBookNavBar, then the entry's icon (word_2E546) and two "
    "labeled fields, confirmed via message dump to be 'BASE VALUE:' "
    "and 'WEIGHT:'.",
    False,
)

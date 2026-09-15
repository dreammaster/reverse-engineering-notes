"""
Names sub_1318D, F5 item-subtype-8's category loop (title dumped as
"WEAPONS", msg 0x8A7A). Structurally identical to
RunClueBookItemCategory (subtype 1, "ARMOR/RINGS"): calls
ShowClueBookItemDetail + sub_1385C, hit-tests region table 0x6976 for
sub-icon clicks, loops until ESC.

This completes identification of all 8 F5 item subtypes: 1=ARMOR/
RINGS, 2=(empty placeholder), 3=JEWELS/ARTIFACTS/UNIQUE ITEMS,
4=MAGIC SCROLLS/QUARTZ, 5=POTIONS, 6=SUPPLIES/FOOD,
7=TRANSPORTATIONS, 8=WEAPONS.

-> RunClueBookWeaponCategory

Run via:
    .\run_ida_script.ps1 name_cluebook_weapon_category.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1318D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunClueBookWeaponCategory", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunClueBookWeaponCategory': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "F5 item-subtype-8 'WEAPONS' clue-book category loop (called from "
    "ShowClueBook), structurally identical to RunClueBookItemCategory "
    "(subtype 1, 'ARMOR/RINGS'): ShowClueBookItemDetail + sub_1385C, "
    "region-table 0x6976 hit-testing for sub-icon clicks, until ESC.",
    False,
)

"""
Names sub_1C809, called from BuildItemUseMessage for special items
([+0xE] bit 0x2000). Checks and pays the item's usage cost, selected
by the item's [+0x10] tag: 1 = gold (g_partyGold, CompareBCD4/SubBCD4
against price table 0xBEA), 2 = NUORE (0x94B7), 3 = MAGIC ORE
(0x94BB), else = a specific item in inventory (IsItemRangeAvailable,
consumed via sub_274B4, not traced). On success sets word_328C6 bit
0x40, the flag BuildItemUseMessage checks to decide whether to apply
the item's effect directly or just show a message.

-> CheckAndPaySpecialItemCost

Run via:
    .\run_ida_script.ps1 name_check_pay_special_cost.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1C809
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckAndPaySpecialItemCost", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckAndPaySpecialItemCost': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks and pays a special item's usage cost by [+0x10] tag: "
    "1=gold, 2=NUORE, 3=MAGIC ORE (all CompareBCD4/SubBCD4 against "
    "price table 0xBEA), else a specific inventory item "
    "(IsItemRangeAvailable). Sets word_328C6 bit 0x40 on success -- "
    "the flag BuildItemUseMessage checks. Called from "
    "BuildItemUseMessage.",
    False,
)

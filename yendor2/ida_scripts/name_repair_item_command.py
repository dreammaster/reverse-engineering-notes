"""
Traced sub_2C010 (called directly from HandleGameCommand) and dumped
its message strings directly from the data segment -- it's the item-
repair minigame. Picks a target character, then rolls RandomInRange(100)
against a pair of thresholds from a table at 0x6B7E (indexed by the
command's "subject" parameter -- word_3293E, the item/item-category
being repaired, x0x14 stride -- combined with a tier offset selected
by the character's own [+0x6A] field, a new party-record field,
plausibly a repair/crafting skill, compared against thresholds
0x32/0x41/0x50/0x5F):

  roll < low threshold  -> "YOUR ATTEMPT TO REPAIR THE ITEM HAS
                            FAILED! THE ITEM WAS DESTROYED." (critical
                            fail, word_328C8 bit 0x4000 set)
  low <= roll <= high   -> "YOUR ATTEMPT TO REPAIR THE ITEM HAS
                            FAILED." (soft fail, item survives)
  roll > high threshold -> "THE ITEM IS REPAIRED." (success,
                            word_328C8 bit 0x8000 set)

-> RepairItemCommand

Run via:
    .\run_ida_script.ps1 name_repair_item_command.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2C010
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RepairItemCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RepairItemCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Item-repair minigame, called directly from HandleGameCommand. "
    "Picks a target character, rolls RandomInRange(100) against a "
    "pair of thresholds from a table at 0x6B7E (indexed by the "
    "item/category being repaired x0x14, plus a tier offset from the "
    "character's own [+0x6A] -- plausibly a repair/crafting skill). "
    "Below the low threshold: critical fail, item destroyed "
    "(word_328C8 |= 0x4000). Between: soft fail, item survives. "
    "Above the high threshold: success, item repaired "
    "(word_328C8 |= 0x8000).",
    False,
)

"""
Names sub_26A75, called once from sub_2621C (the container/inventory
interaction handler, itself called from ShowCharacterInventory and
sub_1869D). Generalizes the already-named per-command eligibility
checks (IsItemEligibleForEnhance/IsItemEligibleForRepair) into one
combined gate covering a whole range of inventory command codes.

ax = word_2E40A (the current key/command code, same convention
documented for other screens' F-key/command dispatch) and bx = the
just-loaded item's catalog record (via LoadItemCatalogRecord, called
immediately before this in sub_2621C).

Command codes <= 8 are always eligible (errorCode stays 0, set at
entry). For codes 9 through 0x14 (20), each is individually gated on
a specific bit of either the item's own catalog flags ([bx+0xC]) or
the separate word_2E548[+2] flags (the same field the item-
classification cluster -- ClassifyItemServiceTier,
GetClassifiedItemStatField -- already uses), with two codes (0xB, 0xD)
adding an extra check against the current party member's own record
([si+0x13E]/[si+0x15C]). Sets errorCode=1 (ineligible, triggering the
caller's FlashStatusWarning) or leaves it 0 (eligible). The individual
command codes' meanings aren't identified. -> IsItemEligibleForCommand

Run via:
    .\run_ida_script.ps1 name_check_item_command_eligibility.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26A75
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsItemEligibleForCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsItemEligibleForCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generalized eligibility gate for inventory command codes "
    "(ax=word_2E40A) against an item's catalog flags (bx, "
    "[bx+0xC]) or word_2E548[+2]. Codes <=8 always pass; 9-0x14 each "
    "check a specific bit, two also checking the current party "
    "member's own record. errorCode=1 if ineligible. Called from "
    "sub_2621C.",
    False,
)

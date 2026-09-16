"""
Names sub_2621C, called from ShowCharacterInventory and sub_1869D --
the click handler for a character's inventory grid slots.

Hit-tests the cursor (offset by word_328BC/word_328C0) against table
0x60EE. With an empty hand: rejects if the clicked slot is empty,
rejects a specific command (word_2E40A==0xB, a "drop"-like command
per the flag check) against a "can't drop" party-record flag
([+0x15C] bit 0x1000), else picks the item up
(PickUpHeldItemFromSlot). With an item already held: loads its
catalog record, checks eligibility for the current command
(IsItemEligibleForCommand, ax=word_2E40A), and on failure
FlashStatusWarning rejects -- on success, continues into
command-specific handling (not fully traced further). Ties the
already-named inventory-slot primitives (GetInventorySlotPtr,
IsItemEligibleForCommand, PickUpHeldItemFromSlot) together into the
actual UI click handler. -> HandleInventorySlotClick

Run via:
    .\run_ida_script.ps1 name_handle_inventory_slot_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2621C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleInventorySlotClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleInventorySlotClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Inventory-grid slot click handler: with an empty hand, picks up "
    "the clicked item (PickUpHeldItemFromSlot) unless a specific "
    "command/flag combination blocks it; with an item held, checks "
    "eligibility for the current command (IsItemEligibleForCommand) "
    "and rejects with FlashStatusWarning on failure. Called from "
    "ShowCharacterInventory and sub_1869D.",
    False,
)

"""
Names sub_271DC, called from `start` and HandleDungeonInput.

Hit-tests the cursor against table 0x636C: indices 0-3 just redraw
DrawResourceCounterPanel (a click on the resource-readout area);
indices 4-9 map to 6 fixed 4-byte slot records (0x9519/951D/9521/
9525/9529/952D -- the same 0x9519 table file-formats.md's
"Quest-item and party-inventory range checks" section already
documents as IsItemRangeAvailable's fixed 6-entry lookup table).

For a clicked slot: if empty and the held item (word_31948) is
placeable there (either the slot's range field is the '/' wildcard
matching a 0x21-0x2E item-id range, or a direct LoadItemCatalogRecord
category-flag check), drops the held item into the slot
(TriggerSoundEvent + store id/quantity, clear held-item state,
ShowResourceDepletedOverlay + refresh HUD). If occupied, swaps: picks
the slot's item back up as the held item and (if the previously-held
item was also present) puts it into the slot instead. Otherwise
(incompatible), FlashStatusWarning and rejects. Reads as the click
handler for a 6-slot item display/storage fixture next to the
resource-counter panel -- narrative purpose (quest altar, display
case, etc.) not identified. -> HandleStatusPanelItemSlotClick

Run via:
    .\run_ida_script.ps1 name_handle_status_panel_item_slot_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x271DC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleStatusPanelItemSlotClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleStatusPanelItemSlotClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Click handler for the resource panel (hit-test indices 0-3, "
    "redraws DrawResourceCounterPanel) and 6 fixed item-display "
    "slots (indices 4-9, table 0x9519 -- the same table "
    "IsItemRangeAvailable uses): places/retrieves/swaps the held "
    "item into a clicked slot if its range/category matches, else "
    "FlashStatusWarning. Called from `start` and HandleDungeonInput.",
    False,
)

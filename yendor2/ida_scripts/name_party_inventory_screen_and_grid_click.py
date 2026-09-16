"""
Names sub_1869D and its inventory-grid-click helper sub_26415.

sub_1869D, called once from `start` right after HandlePortraitClick
sets one of the word_328C6 "portrait expanded" bits (0x4000/0x2000/
0x1000/0x800 -- confirming this is the "click a portrait to open
their inventory" screen). A self-looping (tail-call) polling event
machine dispatching on PollKeyboardInput's errorCode, tying together
essentially the entire party/inventory management surface reachable
elsewhere this session:
- Equipment/bag grid clicks (via sub_26415) -> open/close alternate
  bags or select an item, then ComputeBarterPricingPreview +
  ShowItemPurchaseConfirmPrompt / ConsumeItemChargeResource-style use.
- HandleStatusPanelItemSlotClick, HandleShopCatalogSlotClick (shop
  buy path, gated on word_328C6 bit 0x200), HandlePartyStatusPanelInput,
  RunPartyMemberDetailScreen, HandleInventorySlotClick, TryDropHeldItem,
  HandleItemDropOnPartyPortrait, HandleStatusIconBarClick,
  TryCureAilmentFromIconClick, TryHandleCatalogSlotClick.
- Direct keyboard shortcuts: '1'-'4' (HandlePartyStatusPanelInput),
  SPACE (the confirmed Space-bar sell/enhance/repair-for-gold
  cluster: TrySellItemForGold/TryEnhanceItemForGold/
  TryRepairItemForGold), ESC (cancel, rejecting while holding an
  item), 'P' and 'M' (special item-ability/pickup shortcuts).
- Draws up to 4 expanded party portraits (ShowPartyPortraitForSlot)
  and falls back to HandleGameCommand for any other input, letting
  the player issue ordinary game commands without leaving the screen.
Exits once every portrait-expanded bit clears. -> RunPartyInventoryScreen

sub_26415 (called from RunPartyInventoryScreen and the already-named
ShowCharacterInventory): resolves a click on the equipment/bag grid
(hit-test table 0x60EE, offset by the portrait's clip position).
Opening/closing one of the 3 confirmed alternate bags
(SaveAndCloseContainer/LoadContainerContents, toggling [+0x15C]'s
bag-open bits) if the hit slot is a bag toggle; otherwise, for a
regular item slot, either rejects the click (incompatible with the
held item or the current location, via IsItemTypeAcceptedByLocation)
or stages the item-use parameters (word_3297A/3296C/3296E/32972/
3297C/32978/32976/32974/32970 -- the same globals
ConsumeItemChargeResource reads) and returns errorCode=2 to signal
"item selected, ready for the caller to consume/use it".
-> HandleInventoryGridClick

Run via:
    .\run_ida_script.ps1 name_party_inventory_screen_and_grid_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1869D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunPartyInventoryScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunPartyInventoryScreen': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "The party/inventory management screen, entered when "
    "HandlePortraitClick expands a party portrait. Self-looping "
    "polling event machine tying together the equipment/bag grid "
    "(HandleInventoryGridClick), shop-buy path, party status panels, "
    "held-item drop, ailment-icon cure, and the Space-bar sell/"
    "enhance/repair-for-gold cluster. Called once from `start`.",
    False,
)

ea = 0x26415
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleInventoryGridClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleInventoryGridClick': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Resolves a click on the equipment/bag grid (table 0x60EE): "
    "opens/closes one of the 3 alternate bags, or (for a regular "
    "slot) rejects an incompatible held-item/location combo or "
    "stages item-use parameters and returns errorCode=2 for the "
    "caller to consume/use the selected item. Called from "
    "RunPartyInventoryScreen and ShowCharacterInventory.",
    False,
)

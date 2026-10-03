#ifndef YENDOR23_INVENTORY_H
#define YENDOR23_INVENTORY_H

#include <stdbool.h>
#include <stdint.h>

#include "item.h"
#include "savegame.h"

/*
 * The inventory-screen gates on what an item may do: IsItemEligibleForCommand (yendor2.asm:40383 region),
 * IsContainerTypeCompatible, IsItemDroppable (misnamed: it answers "is dropping BLOCKED") with
 * HasDroppableItemInInventory/Container, and IsItemTypeAcceptedByLocation. All instruction-identical in
 * Chapter 3.
 *
 * Slot codes (party.h partyEquipmentSlot): 0xA main weapon, 0xB, 0xC second weapon/shield slot, 0xD, 0xE/0xF
 * rings, 0x10-0x14 the five 2-byte slots; codes up to 8 are the inventory slots themselves.
 */

/*
 * inventoryEligibleForSlot: may an item with these catalog flags (ItemFieldFlags) and target-entry word 1 be put in
 * slot `code`? partySlotWord is the party record's current content of the code-0xB/0xD-related fields:
 *   occupiedB    the item id at +0x13E (code 0xB)         shieldSlotOccupied  the id at +0x146 (code 0xD)
 *   twoHandedUi  PartyFieldUiFlags bit 0x20 ("a two-handed weapon is equipped")
 * Rules, in the original's order:
 *   code <= 8        always
 *   0xA              flags 0x8000
 *   0xB              an occupied slot always accepts (it is a swap); an empty one needs flags 0x2000
 *   0xC              flags 0x4000; then the entry's word 1 bit 0 (two-handed) requires the 0xD slot to be EMPTY
 *   0xD              flags 0x800 and no two-handed weapon equipped
 *   0xE, 0xF         flags 0x400 (rings)
 *   anything else    flags 0x200 first, then word 1: 0x10 -> 0x8000, 0x11 -> 0x4000, 0x12 -> 0x2000, 0x13 -> 0x1000,
 *                    0x14 (and codes above, and 9) -> 0x800
 */
bool inventoryEligibleForSlot(unsigned code, uint16_t itemFlags, uint16_t entryWord1, bool occupiedB, bool shieldSlotOccupied,
                              bool twoHandedUi);

/*
 * inventoryContainerAccepts: IsContainerTypeCompatible's test. The open container's own flags pick the fit bits it
 * takes -- Bag 0x2000, Box 0x4000, Backpack 0x8000, "any" 0xFFFF, otherwise nothing -- and the item being put in must
 * have one of them in its fit flags (ItemFieldFitFlags). (Only checked for slot codes below 0xA.)
 */
bool inventoryContainerAccepts(uint16_t containerFlags, uint16_t itemFitFlags);

/*
 * inventoryDropBlocked: may the held item NOT be dropped? True when it carries ItemFlagNoDrop, or when it is a
 * container (ItemFlagEquipCode0B, 0x2000) whose contents -- opened in the save's item instances, to a depth of
 * three containers (the held one counts as the first) -- hold such an item. A held id of 0 is not blocked.
 * `contentsRecord` is the held item's extra word (its instance record number).
 */
bool inventoryDropBlocked(const ItemCatalog *catalog, SaveGame *save, uint16_t itemId, uint16_t contentsRecord);

/*
 * inventoryLocationAccepts: the repair/enhance-style "does this spot take that kind of item": a weapon-class item
 * (flags 0xC000) needs the location entry's word 1 bit 0x100, an armour-class item (flags 0x800) bit 0x40; others never.
 */
bool inventoryLocationAccepts(uint16_t itemFlags, uint16_t locationEntryWord1);

#endif

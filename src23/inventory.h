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
bool inventoryDropBlocked(GameKind game, const ItemCatalog *catalog, SaveGame *save, uint16_t itemId, uint16_t contentsRecord);

/*
 * PlaceItemOnGround (yendor2.asm:17974): what dropping an item does to the save. The item itself simply ceases to exist (there is no
 * ground inventory); only a container leaves anything to clean up: its instance record `contentsRecord` and the records of the
 * containers directly inside it and inside those (three levels, the same depth the other container walks use) are zeroed. Containers
 * nested a fourth level down keep their records (the original leaks them). A non-container drops with no effect on the save.
 */
void inventoryDiscardDropped(const ItemCatalog *catalog, SaveGame *save, uint16_t itemId, uint16_t contentsRecord);

/*
 * Chapter 3 adds a second no-drop test: fit flags (ItemFieldFitFlags) bit 0 -- set on every key, the ATHANEUM KEY and
 * the LIT TORCH (a burning torch cannot be dropped). Chapter 2 only tests ItemFlagNoDrop.
 */

/*
 * inventoryLocationAccepts: the repair/enhance-style "does this spot take that kind of item": a weapon-class item
 * (flags 0xC000) needs the location entry's word 1 bit 0x100, an armour-class item (flags 0x800) bit 0x40; others never.
 */
bool inventoryLocationAccepts(uint16_t itemFlags, uint16_t locationEntryWord1);

/*
 * Dropping the held item onto a party member's portrait (HandleItemDropOnPartyPortrait, yendor2.asm:18C80 region;
 * Chapter 3 the same minus one slot) equips or stows it automatically. inventoryPlanAutoEquip decides where.
 *
 * First the carry check: unless the held item is id 0x11 (MAGIC CONTAINER in Chapter 2, weightless; in Chapter 3 the
 * test is left over and treats BROKEN CLUB as weightless -- reproduced), the member's carried weight (+0x118) plus
 * the item's weight must not exceed their carry capacity (+0x56 current value); otherwise it is refused.
 * Then, by the item's flags in this order:
 *   0x8000  main hand +0x13A        (needs it empty; clears wear counter +0xBE; applies the item's stat effect)
 *   0x2000  slot B +0x13E           (empty; the item's container is linked; no stat effect)
 *   0x4000  second hand +0x142      (empty; a two-handed weapon (entry word 1 bit 0) also needs +0x146 empty;
 *                                    clears +0xC0; stat effect)
 *   0x800   +0x146                  (empty and no two-handed weapon (UI flag 0x20); clears +0xC2; stat effect)
 *   0x400   rings +0x14A, else +0x14E
 *   0x200   by the entry's word 1:  0x8000 -> +0x152, 0x4000 -> +0x154, 0x2000 -> +0x156 (Chapter 2 only),
 *                                   0x1000 -> +0x158, 0x800 -> +0x15A    (id only; stat effect)
 * and when its slot is taken, or the item has no such flag, the first empty of the eight inventory slots
 * (+0x11A, +0x11E ... +0x136; the item's container is linked); with all eight full it is refused.
 * Whatever is placed adds its weight (again except id 0x11) and the equipment bonuses are recomputed.
 */
typedef enum {
    AutoEquipRefused,
    AutoEquipEquipment,
    AutoEquipInventory
} AutoEquipKind;

typedef struct {
    AutoEquipKind kind;
    unsigned slotOffset;    /* party record offset of the 2- or 4-byte slot */
    bool storesExtra;       /* the slot has a +2 extra word (4-byte slots) */
    unsigned wearOffset;    /* the wear counter to clear, 0 = none */
    bool appliesStatEffect; /* partyApplyMultiStatEffect for the item */
    bool linksContainer;    /* TryLoadNextContainerLink on the item */
    bool addsWeight;
} AutoEquipPlan;

AutoEquipPlan inventoryPlanAutoEquip(GameKind game, const uint8_t *partyRecord, uint16_t itemId, uint16_t itemWeight,
                                     uint16_t itemFlags, uint16_t entryWord1);

#endif

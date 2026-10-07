#ifndef YENDOR23_CHEST_H
#define YENDOR23_CHEST_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "item.h"
#include "lockcatalog.h"
#include "savegame.h"

/*
 * What a lock record's eight content slots are for the party: a chest to loot, or the stock of a shop (RunShopScreen, HandleShopCatalogSlotClick
 * yendor2.asm:11896, BuildShopCategoryTabList :12932, TriggerShopExitSoundAndPersist :12991; Chapter 3 the same). This closes what used to be listed as
 * the "runtime-filled shop category tab list" (DS:0x5572, zeros in the executable): it is simply the eight item ids of the lock record, copied there by
 * LoadLockState, and the "taken" byte next to it (byte_32DCC, DS:0x556C) is the persisted per-lock byte of CURGAME section 5 (SaveSectionLockAndShopState,
 * one byte per lock id), written back when the shop / chest screen is left. Slot k is bit 0x80 >> k of that byte; a set bit hides the slot.
 *
 * Each remaining slot shows its item id and a value: the pile items 1 / 2 / 3 (GOLD COINS / MAGIC ORE / NUORE) take their amount from the record's gold,
 * ore and nuore words (+0x14, +0x16, +0x18); any other item takes the default charge from its consumable entry (word 2 of the target entry, +4) when the item
 * is a charged consumable, else 0.
 *
 * Taking a slot with an empty hand (when not buying, i.e. looting): a pile adds its amount to the party's gold / ore / nuore counter (the item's flag
 * 0x80 / 0x40 / 0x20 picks which: 94B3 / 94B7 / 94BB = SaveHeaderGold / OreCounter1 / OreCounter2); an ordinary item goes to the cursor with its value as its extra;
 * either way the slot's bit in the taken byte is set. (Putting something back clears the bit again but does not store the item -- the slot shows the
 * record's original content next time, an original quirk not modelled.)
 */
typedef enum { ChestSlotEmpty, ChestSlotItem, ChestSlotGold, ChestSlotOre, ChestSlotNuore } ChestSlotKind;

typedef struct {
    ChestSlotKind kind;
    uint16_t itemId; /* 0 for an empty slot */
    uint16_t value;  /* the pile amount, or the item's extra / charge */
} ChestSlot;

/* The persisted taken byte of the lock (0 when the id is out of range). */
uint8_t chestTakenMask(SaveGame *save, unsigned lockId);

/* The eight slots as the shop / chest screen shows them (BuildShopCategoryTabList). */
void chestSlots(const LockRecord *lock, uint8_t takenMask, const ItemCatalog *catalog, ChestSlot out[LockContentSlots]);

/* Takes slot `slot` (0-7): sets its bit in the lock's persisted byte and, for a pile, adds the amount to the matching counter. False for an empty or taken slot. */
bool chestTake(SaveGame *save, unsigned lockId, const LockRecord *lock, const ItemCatalog *catalog, unsigned slot, ChestSlot *taken);

#endif

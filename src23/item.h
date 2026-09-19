#ifndef YENDOR23_ITEM_H
#define YENDOR23_ITEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The item catalog: a contiguous region of WORLD.DAT holding every item's
 * 58-byte record plus four small tables the records index by byte offset.
 * The game loads it once into EMS (loadWorldDat1, yendor2.asm:3593) and
 * LoadItemCatalogRecord (yendor2.asm:3784) looks records up by id.
 *
 * Item ids are 1-based; the id stored in an inventory slot is
 * page * 256 + number (0x21E = page 2, number 0x1E = SLING). Layouts and
 * field meanings were checked against the real Chapter 2 and Chapter 3
 * WORLD.DAT files (every table offset is in range and aligned; all names
 * decode; prices are valid packed BCD) and, for names, the community item
 * guide in docs23.
 */

enum {
    ItemRecordSize = 58,
    ItemEffectSize = 16,
    ItemEffectPairCount = 4,
    ItemWearableSize = 12, /* target entry for wearables and for weapons */
    ItemWeaponSize = 12,
    ItemConsumableSize = 8,

    ItemCountMax = 759,
    ItemEffectCountMax = 175,
    ItemWearableCountMax = 275,
    ItemWeaponCountMax = 210,
    ItemConsumableCountMax = 250,

    ItemNameLineSize = 13,   /* three fields of 12 characters + NUL */
    ItemNameBufferSize = 39  /* the three lines joined by single spaces, + NUL */
};

/* Offsets within an item record. */
typedef enum {
    ItemFieldTargetOffset = 0x00, /* u16 byte offset into the wearable/weapon/consumable table (see itemTargetKind) */
    ItemFieldEffectOffset = 0x02, /* u16 byte offset into the effect table; 0 = no effect */
    ItemFieldBaseValue = 0x04,    /* Bcd4 gold price ("BASE VALUE:") */
    ItemFieldIcon = 0x08,         /* u16 picture id in category 0x80; +1 when ItemFlagAltIcon is set */
    ItemFieldWeight = 0x0A,       /* u16 ("WEIGHT:") */
    ItemFieldFlags = 0x0C,        /* u16, ItemFlag bits */
    ItemFieldFitFlags = 0x0E,     /* u16, ItemFit bits */
    ItemFieldClass = 0x10,        /* u16 class bits (0x8000 gear, 0x4000 potions ... not fully mapped) */
    ItemFieldName1 = 0x13,        /* 3 x 13-byte text lines, at 0x13, 0x20 and 0x2D */
    ItemFieldName2 = 0x20,
    ItemFieldName3 = 0x2D
} ItemField;

/*
 * ItemFieldFlags bits. The equip bits say which inventory slot codes accept
 * the item (IsItemEligibleForCommand, yendor2.asm:40383; slot codes and
 * offsets are in party.h): a slot code is only valid if the item has its bit.
 */
typedef enum {
    ItemFlagConsumable = 0x0100, /* target record is in the consumable table */
    ItemFlagEquipShort = 0x0200, /* codes 0x10-0x14; ItemTargetSlotFlags picks which */
    ItemFlagEquipRing = 0x0400,  /* codes 0x0E and 0x0F */
    ItemFlagEquipCode0D = 0x0800,
    ItemFlagAltIcon = 0x1000,
    ItemFlagEquipCode0B = 0x2000, /* also marks containers */
    ItemFlagEquipCode0C = 0x4000,
    ItemFlagEquipCode0A = 0x8000
} ItemFlag;

/* ItemFieldFitFlags bits: which containers the item fits in. */
typedef enum {
    ItemFitBag = 0x2000,
    ItemFitBox = 0x4000,
    ItemFitBackpack = 0x8000
} ItemFit;

typedef enum {
    ItemTargetNone,
    ItemTargetWearable,   /* 12 bytes; flags 0x200/0x400/0x800 */
    ItemTargetWeapon,     /* 12 bytes; flags 0x4000/0x8000 */
    ItemTargetConsumable  /* 8 bytes; flag 0x100 */
} ItemTargetKind;

/* Word indices within a 12-byte wearable/weapon target entry. */
enum {
    ItemTargetAbsorption = 0, /* wearables: the "ABSORPTION-" value */
    ItemTargetSlotFlags = 1,  /* wearables: 0x8000/0x4000/0x2000/0x1000/0x800 pick slot codes 0x10-0x14; weapons: skill-type bits */
    ItemTargetBreakItemA = 2, /* replacement item id when it breaks ... */
    ItemTargetBreakChanceA = 3, /* ... and its percent chance (TickEquippedItemDurability) */
    ItemTargetBreakItemB = 4,
    ItemTargetBreakChanceB = 5
};

typedef struct {
    uint32_t itemsOffset; /* WORLD.DAT offset of the item table; the region runs to itemsOffset + totalSize */
    uint32_t totalSize;
    uint16_t itemCount;      /* records the game loads; the tail may not be items */
    uint16_t validItemCount; /* ids 1..validItemCount are real items (Chapter 2 loads 15 extra records of other data) */
    uint16_t effectCount;
    uint16_t wearableCount;
    uint16_t consumableCount;
    uint16_t weaponCount;
} ItemCatalogLayout;

typedef struct {
    GameKind game;
    uint16_t itemCount;
    uint16_t validItemCount;
    uint16_t effectCount;
    uint16_t wearableCount;
    uint16_t consumableCount;
    uint16_t weaponCount;
    uint8_t items[ItemCountMax * ItemRecordSize];
    uint8_t effects[ItemEffectCountMax * ItemEffectSize];
    uint8_t wearables[ItemWearableCountMax * ItemWearableSize];
    uint8_t consumables[ItemConsumableCountMax * ItemConsumableSize];
    uint8_t weapons[ItemWeaponCountMax * ItemWeaponSize];
} ItemCatalog;

const ItemCatalogLayout *itemCatalogLayout(GameKind game);

/*
 * Parses the region that starts at layout->itemsOffset in WORLD.DAT. The
 * region is stored as items, effects, wearables, consumables, weapons.
 * Returns false if size is smaller than layout->totalSize.
 */
bool itemCatalogParse(ItemCatalog *catalog, GameKind game, const uint8_t *region, size_t size);

/* Same, from a whole WORLD.DAT image already in memory. */
bool itemCatalogParseWorldDat(ItemCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

/*
 * Record for a 1-based item id, or NULL for 0 or an id past the loaded
 * table. Like the game's lookup this doesn't stop at validItemCount; iterate
 * up to validItemCount to visit only real items.
 */
const uint8_t *itemCatalogRecord(const ItemCatalog *catalog, unsigned id);

uint16_t itemGetU16(const uint8_t *record, unsigned offset);
const uint8_t *itemBaseValue(const uint8_t *record); /* Bcd4 */

/* One of the three name lines (0-2), trailing spaces trimmed. */
void itemGetNameLine(const uint8_t *record, unsigned line, char out[ItemNameLineSize]);

/* The display name exactly as BuildItemDisplayName joins it ("WOODEN SHIELD +1"). */
void itemGetName(const uint8_t *record, char out[ItemNameBufferSize]);

ItemTargetKind itemTargetKind(const uint8_t *record);

/* The record's wearable/weapon/consumable entry, or NULL if it has none or the offset is out of range. */
const uint8_t *itemTargetEntry(const ItemCatalog *catalog, const uint8_t *record);
uint16_t itemTargetWord(const uint8_t *entry, unsigned word);

/* The record's 16-byte effect entry (4 pairs of party-record field offset and amount), or NULL. */
const uint8_t *itemEffectEntry(const ItemCatalog *catalog, const uint8_t *record);

/* Pairs before the first zero field offset; the game stops there too. */
unsigned itemEffectPairs(const uint8_t *effect);
uint16_t itemEffectField(const uint8_t *effect, unsigned pair); /* offset into a party record */
uint16_t itemEffectAmount(const uint8_t *effect, unsigned pair);

#endif

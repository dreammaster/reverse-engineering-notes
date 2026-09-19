#include "item.h"

#include <string.h>

/*
 * From the record-setup stubs used by loadWorldDat1 (WorldDat_setBlock1/2 and
 * PrepareWorldDat1Block1-4Read) in each game. Every table starts exactly
 * where the previous one ends.
 *
 * Chapter 2's item table is read in two pieces (699 + 60 records) that are
 * adjacent in the file; Chapter 3 reads a single piece. Chapter 2's last
 * real item is id 744 (SCROLL OF RESURRECTION): the 15 records after it are
 * slack in the game's block size and hold unrelated data.
 */
static const ItemCatalogLayout g_layoutYendor2 = {
    0x71138, 54402, 759, 744, 175, 275, 250, 190,
};

static const ItemCatalogLayout g_layoutYendor3 = {
    0x83EE8, 45346, 631, 631, 148, 221, 151, 210,
};

const ItemCatalogLayout *itemCatalogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool itemCatalogParse(ItemCatalog *catalog, GameKind game, const uint8_t *region, size_t size) {
    const ItemCatalogLayout *layout = itemCatalogLayout(game);
    if (!layout || size < layout->totalSize) {
        return false;
    }

    memset(catalog, 0, sizeof(*catalog));
    catalog->game = game;
    catalog->itemCount = layout->itemCount;
    catalog->validItemCount = layout->validItemCount;
    catalog->effectCount = layout->effectCount;
    catalog->wearableCount = layout->wearableCount;
    catalog->consumableCount = layout->consumableCount;
    catalog->weaponCount = layout->weaponCount;

    size_t position = 0;
    size_t bytes = (size_t)layout->itemCount * ItemRecordSize;
    memcpy(catalog->items, region + position, bytes);
    position += bytes;
    bytes = (size_t)layout->effectCount * ItemEffectSize;
    memcpy(catalog->effects, region + position, bytes);
    position += bytes;
    bytes = (size_t)layout->wearableCount * ItemWearableSize;
    memcpy(catalog->wearables, region + position, bytes);
    position += bytes;
    bytes = (size_t)layout->consumableCount * ItemConsumableSize;
    memcpy(catalog->consumables, region + position, bytes);
    position += bytes;
    bytes = (size_t)layout->weaponCount * ItemWeaponSize;
    memcpy(catalog->weapons, region + position, bytes);
    return true;
}

bool itemCatalogParseWorldDat(ItemCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const ItemCatalogLayout *layout = itemCatalogLayout(game);
    if (!layout || size < (size_t)layout->itemsOffset + layout->totalSize) {
        return false;
    }
    return itemCatalogParse(catalog, game, worldDat + layout->itemsOffset, layout->totalSize);
}

const uint8_t *itemCatalogRecord(const ItemCatalog *catalog, unsigned id) {
    if (id == 0 || id > catalog->itemCount) {
        return NULL;
    }
    return catalog->items + (size_t)(id - 1) * ItemRecordSize;
}

uint16_t itemGetU16(const uint8_t *record, unsigned offset) {
    return (uint16_t)(record[offset] | (record[offset + 1] << 8));
}

const uint8_t *itemBaseValue(const uint8_t *record) {
    return record + ItemFieldBaseValue;
}

static size_t trimmedLength(const char *text, size_t length) {
    while (length > 0 && text[length - 1] == ' ') {
        length--;
    }
    return length;
}

static size_t lineLength(const uint8_t *record, unsigned offset) {
    size_t length = 0;
    while (length < ItemNameLineSize - 1 && record[offset + length] != 0) {
        length++;
    }
    return length;
}

void itemGetNameLine(const uint8_t *record, unsigned line, char out[ItemNameLineSize]) {
    static const unsigned offsets[3] = {ItemFieldName1, ItemFieldName2, ItemFieldName3};
    if (line >= 3) {
        out[0] = '\0';
        return;
    }
    size_t length = trimmedLength((const char *)record + offsets[line], lineLength(record, offsets[line]));
    memcpy(out, record + offsets[line], length);
    out[length] = '\0';
}

void itemGetName(const uint8_t *record, char out[ItemNameBufferSize]) {
    static const unsigned offsets[3] = {ItemFieldName1, ItemFieldName2, ItemFieldName3};
    size_t length = 0;

    /* Copy a line, add a separator, copy the next, trimming trailing spaces after each. */
    for (unsigned line = 0; line < 3; line++) {
        if (line > 0) {
            out[length++] = ' ';
        }
        size_t lineLen = lineLength(record, offsets[line]);
        memcpy(out + length, record + offsets[line], lineLen);
        length = trimmedLength(out, length + lineLen);
    }
    out[length] = '\0';
}

ItemTargetKind itemTargetKind(const uint8_t *record) {
    uint16_t flags = itemGetU16(record, ItemFieldFlags);
    if (flags & (ItemFlagEquipShort | ItemFlagEquipRing | ItemFlagEquipCode0D)) {
        return ItemTargetWearable;
    }
    if (flags & (ItemFlagEquipCode0C | ItemFlagEquipCode0A)) {
        return ItemTargetWeapon;
    }
    if (flags & ItemFlagConsumable) {
        return ItemTargetConsumable;
    }
    return ItemTargetNone;
}

const uint8_t *itemTargetEntry(const ItemCatalog *catalog, const uint8_t *record) {
    const uint8_t *table;
    size_t entrySize;
    size_t tableSize;

    switch (itemTargetKind(record)) {
    case ItemTargetWearable:
        table = catalog->wearables;
        entrySize = ItemWearableSize;
        tableSize = (size_t)catalog->wearableCount * ItemWearableSize;
        break;
    case ItemTargetWeapon:
        table = catalog->weapons;
        entrySize = ItemWeaponSize;
        tableSize = (size_t)catalog->weaponCount * ItemWeaponSize;
        break;
    case ItemTargetConsumable:
        table = catalog->consumables;
        entrySize = ItemConsumableSize;
        tableSize = (size_t)catalog->consumableCount * ItemConsumableSize;
        break;
    default:
        return NULL;
    }

    size_t offset = itemGetU16(record, ItemFieldTargetOffset);
    if (offset + entrySize > tableSize) {
        return NULL;
    }
    return table + offset;
}

uint16_t itemTargetWord(const uint8_t *entry, unsigned word) {
    return itemGetU16(entry, word * 2);
}

const uint8_t *itemEffectEntry(const ItemCatalog *catalog, const uint8_t *record) {
    size_t offset = itemGetU16(record, ItemFieldEffectOffset);
    if (offset == 0 || offset + ItemEffectSize > (size_t)catalog->effectCount * ItemEffectSize) {
        return NULL;
    }
    return catalog->effects + offset;
}

unsigned itemEffectPairs(const uint8_t *effect) {
    unsigned count = 0;
    while (count < ItemEffectPairCount && itemGetU16(effect, count * 4) != 0) {
        count++;
    }
    return count;
}

uint16_t itemEffectField(const uint8_t *effect, unsigned pair) {
    return pair < ItemEffectPairCount ? itemGetU16(effect, pair * 4) : 0;
}

uint16_t itemEffectAmount(const uint8_t *effect, unsigned pair) {
    return pair < ItemEffectPairCount ? itemGetU16(effect, pair * 4 + 2) : 0;
}

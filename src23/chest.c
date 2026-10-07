#include "chest.h"

#include "bcd4.h"

uint8_t chestTakenMask(SaveGame *save, unsigned lockId) {
    uint8_t *byte = lockId ? saveGameRecord(save, SaveSectionLockAndShopState, lockId - 1) : NULL;
    return byte ? *byte : 0;
}

static unsigned itemFlags(const uint8_t *record) {
    return (unsigned)record[ItemFieldFlags] | ((unsigned)record[ItemFieldFlags + 1] << 8);
}

static ChestSlotKind pileKind(const ItemCatalog *catalog, unsigned itemId) {
    const uint8_t *record = itemCatalogRecord(catalog, itemId);
    if (!record) {
        return ChestSlotItem;
    }
    unsigned flags = itemFlags(record);
    return flags & 0x80 ? ChestSlotGold : flags & 0x40 ? ChestSlotOre : flags & 0x20 ? ChestSlotNuore : ChestSlotItem;
}

void chestSlots(const LockRecord *lock, uint8_t takenMask, const ItemCatalog *catalog, ChestSlot out[LockContentSlots]) {
    for (unsigned i = 0; i < LockContentSlots; i++) {
        ChestSlot *slot = &out[i];
        slot->kind = ChestSlotEmpty;
        slot->itemId = 0;
        slot->value = 0;
        uint16_t id = lock->items[i];
        if ((takenMask & (0x80u >> i)) || id == 0) {
            continue;
        }
        slot->itemId = id;
        if (id == 1) {
            slot->value = lock->gold;
        } else if (id == 3) {
            slot->value = lock->nuore;
        } else if (id == 2) {
            slot->value = lock->magicOre;
        } else {
            const uint8_t *record = itemCatalogRecord(catalog, id);
            if (record && (itemFlags(record) & ItemFlagConsumable)) {
                const uint8_t *entry = itemTargetEntry(catalog, record);
                if (entry && (itemTargetWord(entry, ItemTargetSlotFlags) & 1)) {
                    slot->value = itemTargetWord(entry, 2);
                }
            }
        }
        slot->kind = pileKind(catalog, id);
    }
}

bool chestTake(SaveGame *save, unsigned lockId, const LockRecord *lock, const ItemCatalog *catalog, unsigned slot, ChestSlot *taken) {
    uint8_t *byte = lockId ? saveGameRecord(save, SaveSectionLockAndShopState, lockId - 1) : NULL;
    if (!byte || slot >= LockContentSlots) {
        return false;
    }
    ChestSlot slots[LockContentSlots];
    chestSlots(lock, *byte, catalog, slots);
    if (slots[slot].kind == ChestSlotEmpty) {
        return false;
    }
    *byte |= (uint8_t)(0x80u >> slot);
    switch (slots[slot].kind) {
    case ChestSlotGold:
        bcd4AddU16(saveHeaderBcd4(save, SaveHeaderGold), slots[slot].value);
        break;
    case ChestSlotOre:
        bcd4AddU16(saveHeaderBcd4(save, SaveHeaderOreCounter1), slots[slot].value);
        break;
    case ChestSlotNuore:
        bcd4AddU16(saveHeaderBcd4(save, SaveHeaderOreCounter2), slots[slot].value);
        break;
    default:
        break;
    }
    if (taken) {
        *taken = slots[slot];
    }
    return true;
}

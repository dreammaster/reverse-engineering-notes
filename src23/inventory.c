#include "inventory.h"

#include "party.h"

bool inventoryEligibleForSlot(unsigned code, uint16_t itemFlags, uint16_t entryWord1, bool occupiedB, bool shieldSlotOccupied,
                              bool twoHandedUi) {
    if (code <= 8) {
        return true;
    }
    switch (code) {
    case 0xA:
        return (itemFlags & 0x8000) != 0;
    case 0xB:
        return occupiedB || (itemFlags & 0x2000);
    case 0xC:
        if (!(itemFlags & 0x4000)) {
            return false;
        }
        if (!(entryWord1 & 1)) {
            return true;
        }
        return !shieldSlotOccupied;
    case 0xD:
        return (itemFlags & 0x800) && !twoHandedUi;
    case 0xE:
    case 0xF:
        return (itemFlags & 0x400) != 0;
    default:
        break;
    }
    if (!(itemFlags & 0x200)) {
        return false;
    }
    switch (code) {
    case 0x10:
        return (entryWord1 & 0x8000) != 0;
    case 0x11:
        return (entryWord1 & 0x4000) != 0;
    case 0x12:
        return (entryWord1 & 0x2000) != 0;
    case 0x13:
        return (entryWord1 & 0x1000) != 0;
    default:
        return (entryWord1 & 0x800) != 0;
    }
}

bool inventoryContainerAccepts(uint16_t containerFlags, uint16_t itemFitFlags) {
    uint16_t mask = 0;
    if (containerFlags & ItemFlagContainerBag) {
        mask = ItemFitBag;
    } else if (containerFlags & ItemFlagContainerBox) {
        mask = ItemFitBox;
    } else if (containerFlags & ItemFlagContainerBackpack) {
        mask = ItemFitBackpack;
    } else if (containerFlags & ItemFlagContainerAny) {
        mask = 0xFFFF;
    }
    return (itemFitFlags & mask) != 0;
}

static bool itemFlagsFor(const ItemCatalog *catalog, uint16_t id, uint16_t *flags) {
    const uint8_t *record = itemCatalogRecord(catalog, id);
    if (!record) {
        return false;
    }
    *flags = itemGetU16(record, ItemFieldFlags);
    return true;
}

/* One level of HasDroppableItemInInventory/Container: contents of instance record `number`; `level` 1-3. */
static bool contentsBlocked(GameKind game, const ItemCatalog *catalog, SaveGame *save, unsigned number, unsigned level) {
    uint8_t *contents = saveGameRecord(save, SaveSectionItemInstances, number);
    if (!contents) {
        return false;
    }
    for (unsigned slot = 1; slot <= 8; slot++) {
        const uint8_t *entry = inventoryGroupSlot(contents, slot);
        uint16_t id = itemSlotId(entry), flags;
        if (id == 0 || !itemFlagsFor(catalog, id, &flags)) {
            continue;
        }
        if ((flags & ItemFlagNoDrop) || (game == GameYendor3 && (itemGetU16(itemCatalogRecord(catalog, id), ItemFieldFitFlags) & 1))) {
            return true;
        }
        if (level < 3 && (flags & ItemFlagEquipCode0B) && contentsBlocked(game, catalog, save, itemSlotExtra(entry), level + 1)) {
            return true;
        }
    }
    return false;
}

bool inventoryDropBlocked(GameKind game, const ItemCatalog *catalog, SaveGame *save, uint16_t itemId, uint16_t contentsRecord) {
    uint16_t flags;
    if (itemId == 0 || !itemFlagsFor(catalog, itemId, &flags)) {
        return false;
    }
    if (flags & ItemFlagNoDrop) {
        return true;
    }
    if (game == GameYendor3 && (itemGetU16(itemCatalogRecord(catalog, itemId), ItemFieldFitFlags) & 1)) {
        return true;
    }
    return (flags & ItemFlagEquipCode0B) && contentsBlocked(game, catalog, save, contentsRecord, 1);
}

bool inventoryLocationAccepts(uint16_t itemFlags, uint16_t locationEntryWord1) {
    if (itemFlags & 0xC000) {
        return (locationEntryWord1 & 0x100) != 0;
    }
    if (itemFlags & 0x800) {
        return (locationEntryWord1 & 0x40) != 0;
    }
    return false;
}

AutoEquipPlan inventoryPlanAutoEquip(GameKind game, const uint8_t *partyRecord, uint16_t itemId, uint16_t itemWeight,
                                     uint16_t itemFlags, uint16_t entryWord1) {
    AutoEquipPlan plan = {AutoEquipRefused, 0, false, 0, false, false, itemId != 0x11};
    if (itemId != 0x11 &&
        (int)partyGetU16(partyRecord, PartyFieldInventory) + itemWeight > (int16_t)partyGetStat(partyRecord, PartyStatCarryCapacity)) {
        return plan;
    }
    uint16_t uiFlags = partyGetU16(partyRecord, PartyFieldUiFlags);
    bool addsWeight = plan.addsWeight;
    bool placed = false;
    if (itemFlags & 0x8000) {
        if (partyGetU16(partyRecord, 0x13A) == 0) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x13A, true, PartyFieldWearMain, true, false, addsWeight};
            placed = true;
        }
    } else if (itemFlags & 0x2000) {
        if (partyGetU16(partyRecord, 0x13E) == 0) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x13E, true, 0, false, true, addsWeight};
            placed = true;
        }
    } else if (itemFlags & 0x4000) {
        if (partyGetU16(partyRecord, 0x142) == 0 && !((entryWord1 & 1) && partyGetU16(partyRecord, 0x146) != 0)) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x142, true, PartyFieldWearSecond, true, false, addsWeight};
            placed = true;
        }
    } else if (itemFlags & 0x800) {
        if (partyGetU16(partyRecord, 0x146) == 0 && !(uiFlags & 0x20)) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x146, true, PartyFieldWearThird, true, false, addsWeight};
            placed = true;
        }
    } else if (itemFlags & 0x400) {
        if (partyGetU16(partyRecord, 0x14A) == 0) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x14A, true, 0, true, false, addsWeight};
            placed = true;
        } else if (partyGetU16(partyRecord, 0x14E) == 0) {
            plan = (AutoEquipPlan){AutoEquipEquipment, 0x14E, true, 0, true, false, addsWeight};
            placed = true;
        }
    } else if (itemFlags & 0x200) {
        unsigned offset = 0;
        if (entryWord1 & 0x8000) {
            offset = 0x152;
        } else if (entryWord1 & 0x4000) {
            offset = 0x154;
        } else if ((entryWord1 & 0x2000) && game != GameYendor3) {
            offset = 0x156;
        } else if (entryWord1 & 0x1000) {
            offset = 0x158;
        } else if (entryWord1 & 0x800) {
            offset = 0x15A;
        }
        if (offset && partyGetU16(partyRecord, offset) == 0) {
            plan = (AutoEquipPlan){AutoEquipEquipment, offset, false, 0, true, false, addsWeight};
            placed = true;
        }
    }
    if (placed) {
        return plan;
    }
    for (unsigned slot = 0; slot < 8; slot++) {
        unsigned offset = PartyFieldInventory + 2 + slot * ItemSlotSize;
        if (partyGetU16(partyRecord, offset) == 0) {
            return (AutoEquipPlan){AutoEquipInventory, offset, true, 0, false, true, addsWeight};
        }
    }
    plan.kind = AutoEquipRefused;
    return plan;
}

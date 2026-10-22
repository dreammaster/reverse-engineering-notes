#include "container.h"

#include <string.h>

#include "party.h"

unsigned containerAllocate(SaveGame *save) {
    unsigned record;
    unsigned head = saveHeaderGetU16(save, ContainerHeaderFreeHead);
    if (head != 0) {
        const uint8_t *free = saveGameRecord(save, SaveSectionItemInstances, head);
        record = head;
        saveHeaderSetU16(save, ContainerHeaderFreeHead, free ? partyGetU16(free, 0) : 0);
    } else {
        record = saveHeaderGetU16(save, ContainerHeaderNextRecord);
        saveHeaderSetU16(save, ContainerHeaderNextRecord, (uint16_t)(record + 1));
    }
    uint8_t *contents = saveGameRecord(save, SaveSectionItemInstances, record);
    if (contents) {
        memset(contents, 0, InventoryGroupSize);
    }
    return record;
}

void containerFree(SaveGame *save, unsigned record) {
    uint8_t *contents = saveGameRecord(save, SaveSectionItemInstances, record);
    if (!contents) {
        return;
    }
    memset(contents, 0, InventoryGroupSize);
    partySetU16(contents, 0, saveHeaderGetU16(save, ContainerHeaderFreeHead));
    saveHeaderSetU16(save, ContainerHeaderFreeHead, (uint16_t)record);
}

void containerFlushOpen(SaveGame *save, uint8_t *partyRecord) {
    for (unsigned bag = 0; bag < 3; bag++) {
        const uint8_t *marker = partyBagMarker(partyRecord, bag);
        uint8_t *contents = partyGetU16(marker, 0) ? saveGameRecord(save, SaveSectionItemInstances, partyGetU16(marker, 2)) : NULL;
        if (contents) {
            memcpy(contents, marker + 4, InventoryGroupSize);
        }
    }
}

unsigned containerWeight(SaveGame *save, unsigned record) {
    const uint8_t *contents = record ? saveGameRecord(save, SaveSectionItemInstances, record) : NULL;
    return contents ? partyGetU16(contents, 0) : 0;
}

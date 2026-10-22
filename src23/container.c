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

unsigned containerWeight(SaveGame *save, unsigned record) {
    const uint8_t *contents = record ? saveGameRecord(save, SaveSectionItemInstances, record) : NULL;
    return contents ? partyGetU16(contents, 0) : 0;
}

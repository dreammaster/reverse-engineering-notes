#include "savegame.h"

#include <string.h>

/*
 * Section offsets/sizes come from the record-setup stubs (yendor2.asm:
 * 27da8-27e3a and 27d8a, Chapter 3's equivalents at the same names) and
 * the InitGlobals constants that size them. Chapter 2 adds up to exactly
 * its 77,509-byte file; Chapter 3's chain closes at 81,037.
 */
static const SaveLayout g_layoutYendor2 = {
    SaveFileSizeYendor2,
    {
        [SaveSectionHeaderAndParty] = {0x0000, 5000, 500, 10},
        [SaveSectionExploredMap] = {0x1388, 14400, 100, 144},
        [SaveSectionItemInstances] = {0x4BC8, 44064, 34, 1296},
        [SaveSectionEventState] = {0xF7E8, 644, 1, 644},
        [SaveSectionLockAndShopState] = {0xFA6C, 608, 1, 608},
        [SaveSectionMonsterSpawnFlags] = {0xFCCC, 313, 1, 313},
        [SaveSectionMonsters] = {0xFE05, 12480, 156, 80},
    },
};

static const SaveLayout g_layoutYendor3 = {
    SaveFileSizeYendor3,
    {
        [SaveSectionHeaderAndParty] = {0x0000, 5000, 500, 10},
        [SaveSectionExploredMap] = {0x1388, 16800, 100, 168},
        [SaveSectionItemInstances] = {0x5528, 44064, 34, 1296},
        [SaveSectionEventState] = {0x10148, 1059, 1, 1059},
        [SaveSectionLockAndShopState] = {0x1056B, 1008, 1, 1008},
        [SaveSectionMonsterSpawnFlags] = {0x1095B, 626, 1, 626},
        [SaveSectionMonsters] = {0x10BCD, 12480, 156, 80},
    },
};

const SaveLayout *saveLayoutFor(SaveGameKind kind) {
    switch (kind) {
    case SaveGameYendor2:
        return &g_layoutYendor2;
    case SaveGameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

void saveGameInit(SaveGame *save, SaveGameKind kind) {
    memset(save, 0, sizeof(*save));
    save->kind = kind;
}

bool saveGameLoad(SaveGame *save, const uint8_t *data, size_t size) {
    SaveGameKind kind;
    if (size == SaveFileSizeYendor2) {
        kind = SaveGameYendor2;
    } else if (size == SaveFileSizeYendor3) {
        kind = SaveGameYendor3;
    } else {
        return false;
    }
    saveGameInit(save, kind);
    memcpy(save->bytes, data, size);
    return true;
}

size_t saveGameStore(const SaveGame *save, uint8_t *out, size_t capacity) {
    size_t size = saveLayoutFor(save->kind)->totalSize;
    if (capacity < size) {
        return 0;
    }
    memcpy(out, save->bytes, size);
    return size;
}

uint8_t *saveGameSection(SaveGame *save, SaveSection section) {
    if ((unsigned)section >= SaveSectionCount) {
        return NULL;
    }
    return save->bytes + saveLayoutFor(save->kind)->sections[section].offset;
}

uint8_t *saveGameRecord(SaveGame *save, SaveSection section, unsigned index) {
    if ((unsigned)section >= SaveSectionCount) {
        return NULL;
    }
    const SaveSectionInfo *info = &saveLayoutFor(save->kind)->sections[section];
    if (index >= info->recordCount) {
        return NULL;
    }
    return save->bytes + info->offset + (size_t)index * info->recordSize;
}

uint8_t *saveGamePartyRecord(SaveGame *save, unsigned index) {
    if (index >= SavePartyRecordCount) {
        return NULL;
    }
    return saveGameRecord(save, SaveSectionHeaderAndParty, 1 + index);
}

uint8_t *saveGamePartyRecordById(SaveGame *save, unsigned id) {
    if (id == 0) {
        return NULL;
    }
    return saveGamePartyRecord(save, id - 1);
}

uint16_t saveHeaderGetU16(const SaveGame *save, unsigned offset) {
    const uint8_t *p = save->bytes + offset;
    return (uint16_t)(p[0] | (p[1] << 8));
}

void saveHeaderSetU16(SaveGame *save, unsigned offset, uint16_t value) {
    uint8_t *p = save->bytes + offset;
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

uint8_t *saveHeaderBcd4(SaveGame *save, unsigned offset) {
    return save->bytes + offset;
}

uint16_t saveGetPartySlot(const SaveGame *save, unsigned slot) {
    if (slot >= SavePartyMemberSlots) {
        return 0;
    }
    return saveHeaderGetU16(save, SaveHeaderPartySlots + slot * 2);
}

void saveGetName(const SaveGame *save, char out[SaveNameBufferSize]) {
    size_t length = 0;
    while (length < SaveNameMaxLength && save->bytes[SaveHeaderName + length] != 0) {
        length++;
    }
    memcpy(out, save->bytes + SaveHeaderName, length);
    out[length] = '\0';
}

bool saveSetName(SaveGame *save, const char *name) {
    size_t length = strlen(name);
    if (length > SaveNameMaxLength) {
        return false;
    }
    memcpy(save->bytes + SaveHeaderName, name, length + 1);
    return true;
}

const char *saveFacingName(uint16_t facing) {
    switch (facing) {
    case SaveFacingNorth:
        return "NORTH";
    case SaveFacingSouth:
        return "SOUTH";
    case SaveFacingEast:
        return "EAST";
    case SaveFacingWest:
        return "WEST";
    }
    return NULL;
}

bool saveSlotFileName(char out[SaveSlotFileNameSize], unsigned slot) {
    if (slot < 1 || slot > SaveSlotCount) {
        return false;
    }
    memcpy(out, "SAVGAME", 7);
    out[7] = (char)('0' + slot);
    out[8] = '\0';
    return true;
}

#include "monsterpool.h"

#include <string.h>

static uint8_t *spawnFlagByte(SaveGame *save, unsigned typeId) {
    return saveGameRecord(save, SaveSectionMonsterSpawnFlags, typeId / 8);
}

bool monsterSpawnFlagTest(SaveGame *save, GameKind game, unsigned typeId) {
    (void)game;
    uint8_t *byte = spawnFlagByte(save, typeId);
    if (!byte) {
        return false;
    }
    return (*byte >> (7 - (typeId % 8))) & 1;
}

void monsterSpawnFlagSet(SaveGame *save, GameKind game, unsigned typeId) {
    (void)game;
    uint8_t *byte = spawnFlagByte(save, typeId);
    if (byte) {
        *byte |= (uint8_t)(0x80 >> (typeId % 8));
    }
}

void monsterSpawnFlagClear(SaveGame *save, GameKind game, unsigned typeId) {
    (void)game;
    uint8_t *byte = spawnFlagByte(save, typeId);
    if (byte) {
        *byte &= (uint8_t)~(0x80 >> (typeId % 8));
    }
}

unsigned monsterPoolRefreshWindow(uint8_t *pool, DungeonGrid *grid, SaveGame *save) {
    unsigned stillOccupied = 0;

    for (unsigned slot = 0; slot < MonsterPoolSize; slot++) {
        uint8_t *record = pool + (size_t)slot * MonsterRecordSize;
        uint16_t type = monsterGetU16(record, MonsterFieldType);
        if (type == 0) {
            continue;
        }

        int worldX = monsterGetU16(record, MonsterFieldWorldX);
        int worldY = monsterGetU16(record, MonsterFieldWorldY);
        /*
         * The original's own bounds check (yendor2.asm:29515 on) is
         * inclusive on the high side (worldPos <= origin + 78), one
         * cell wider than DungeonGridSize -- a real 79-wide tracked
         * range, not an off-by-one to correct. Kept exactly for the
         * survives-vs-despawns decision; the relative offset it can
         * produce (78) simply has no DungeonGrid cell to overlay,
         * handled below by dungeonGridCellMutable returning NULL.
         */
        bool inWindow = worldX >= grid->originCol && worldX <= grid->originCol + DungeonGridSize &&
                         worldY >= grid->originRow && worldY <= grid->originRow + DungeonGridSize;

        if (!inWindow) {
            memset(record, 0, MonsterRecordSize);
            if (save) {
                monsterSpawnFlagClear(save, grid->game, type);
            }
            continue;
        }

        int relRow = worldY - grid->originRow;
        int relCol = worldX - grid->originCol;
        monsterSetU16(record, MonsterFieldCell, (uint16_t)(relRow * 0x270 + relCol * 8));

        DungeonGridCell *cell = dungeonGridCellMutable(grid, relRow, relCol);
        if (cell) {
            cell->reserved4 = type;
            cell->flags |= DungeonGridCellFlagOverlay;
        }
        stillOccupied++;
    }

    return stillOccupied;
}

/*
 * yendor2/yendor3's ida_scripts/dump_spawn_offset_tables.py: confirmed
 * byte-for-byte identical between both games. Grouped by dy (see the
 * MonsterSpawnOffsetCount doc comment); North is the party's own
 * forward direction table, South/East/West the other three facings.
 */
static const MonsterSpawnOffset kOffsetsNorth[MonsterSpawnOffsetCount] = {
    {-8, -6}, {-7, -6}, {-6, -6}, {-5, -6}, {-4, -6}, {-3, -6}, {-2, -6}, {-1, -6}, {0, -6},  {1, -6},  {2, -6},
    {3, -6},  {4, -6},  {5, -6},  {6, -6},  {7, -6},  {8, -6},  {-8, -5}, {-7, -5}, {-6, -5}, {-5, -5}, {-4, -5},
    {-3, -5}, {-2, -5}, {-1, -5}, {0, -5},  {1, -5},  {2, -5},  {3, -5},  {4, -5},  {5, -5},  {6, -5},  {7, -5},
    {8, -5},  {-2, -4}, {-1, -4}, {0, -4},  {1, -4},  {2, -4},  {-1, -3}, {0, -3},  {1, -3},  {-1, -2}, {0, -2},
    {1, -2},  {-1, -1}, {0, -1},  {1, -1},  {-1, 0},  {0, 0},   {1, 0},
};

static const MonsterSpawnOffset kOffsetsSouth[MonsterSpawnOffsetCount] = {
    {8, 6},   {7, 6},   {6, 6},   {5, 6},   {4, 6},   {3, 6},   {2, 6},   {1, 6},   {0, 6},  {-1, 6}, {-2, 6},
    {-3, 6},  {-4, 6},  {-5, 6},  {-6, 6},  {-7, 6},  {-8, 6},  {8, 5},   {7, 5},   {6, 5},  {5, 5},  {4, 5},
    {3, 5},   {2, 5},   {1, 5},   {0, 5},   {-1, 5},  {-2, 5},  {-3, 5},  {-4, 5},  {-5, 5}, {-6, 5}, {-7, 5},
    {-8, 5},  {2, 4},   {1, 4},   {0, 4},   {-1, 4},  {-2, 4},  {1, 3},   {0, 3},   {-1, 3}, {1, 2},  {0, 2},
    {-1, 2},  {1, 1},   {0, 1},   {-1, 1},  {1, 0},   {0, 0},   {-1, 0},
};

static const MonsterSpawnOffset kOffsetsEast[MonsterSpawnOffsetCount] = {
    {6, -8}, {6, -7}, {6, -6}, {6, -5}, {6, -4}, {6, -3}, {6, -2}, {6, -1}, {6, 0}, {6, 1}, {6, 2},
    {6, 3},  {6, 4},  {6, 5},  {6, 6},  {6, 7},  {6, 8},  {5, -8}, {5, -7}, {5, -6}, {5, -5}, {5, -4},
    {5, -3}, {5, -2}, {5, -1}, {5, 0},  {5, 1},  {5, 2},  {5, 3},  {5, 4},  {5, 5}, {5, 6}, {5, 7},
    {5, 8},  {4, -2}, {4, -1}, {4, 0},  {4, 1},  {4, 2},  {3, -1}, {3, 0},  {3, 1}, {2, -1}, {2, 0},
    {2, 1},  {1, -1}, {1, 0},  {1, 1},  {0, -1}, {0, 0},  {0, 1},
};

static const MonsterSpawnOffset kOffsetsWest[MonsterSpawnOffsetCount] = {
    {-6, 8},  {-6, 7},  {-6, 6},  {-6, 5},  {-6, 4},  {-6, 3},  {-6, 2},  {-6, 1},  {-6, 0}, {-6, -1}, {-6, -2},
    {-6, -3}, {-6, -4}, {-6, -5}, {-6, -6}, {-6, -7}, {-6, -8}, {-5, 8},  {-5, 7},  {-5, 6}, {-5, 5},  {-5, 4},
    {-5, 3},  {-5, 2},  {-5, 1},  {-5, 0},  {-5, -1}, {-5, -2}, {-5, -3}, {-5, -4}, {-5, -5}, {-5, -6}, {-5, -7},
    {-5, -8}, {-4, 2},  {-4, 1},  {-4, 0},  {-4, -1}, {-4, -2}, {-3, 1},  {-3, 0},  {-3, -1}, {-2, 1},  {-2, 0},
    {-2, -1}, {-1, 1},  {-1, 0},  {-1, -1}, {0, 1},   {0, 0},   {0, -1},
};

const MonsterSpawnOffset *monsterSpawnOffsetTable(uint16_t facing) {
    switch (facing) {
    case SaveFacingNorth: return kOffsetsNorth;
    case SaveFacingSouth: return kOffsetsSouth;
    case SaveFacingEast: return kOffsetsEast;
    case SaveFacingWest: return kOffsetsWest;
    default: return NULL;
    }
}

int monsterPoolSpawn(uint8_t *pool, const MonsterCatalog *catalog, SaveGame *save, GameKind game, uint16_t facing,
                      uint16_t partyWorldX, uint16_t partyWorldY, uint16_t gridOriginRow, uint16_t gridOriginCol,
                      unsigned viewportIndex, unsigned typeId, RandomState *rng) {
    const MonsterSpawnOffset *offsets = monsterSpawnOffsetTable(facing);
    if (!offsets || viewportIndex >= MonsterSpawnOffsetCount) {
        return -1;
    }

    int slot = -1;
    for (unsigned i = 0; i < MonsterPoolSize; i++) {
        if (monsterGetU16(pool + (size_t)i * MonsterRecordSize, MonsterFieldType) == 0) {
            slot = (int)i;
            break;
        }
    }
    if (slot < 0) {
        return -1;
    }

    uint8_t *record = pool + (size_t)slot * MonsterRecordSize;
    if (!monsterRecordSpawn(record, catalog, typeId)) {
        return -1;
    }

    const MonsterSpawnOffset *offset = &offsets[viewportIndex];
    uint16_t worldX = (uint16_t)(partyWorldX + offset->dx);
    uint16_t worldY = (uint16_t)(partyWorldY + offset->dy);
    monsterRecordPlace(record, worldX, worldY, gridOriginRow, gridOriginCol);
    monsterRecordStartAnimation(record, randomInRange(rng, 5));

    if (save) {
        monsterSpawnFlagSet(save, game, typeId);
    }
    return slot;
}

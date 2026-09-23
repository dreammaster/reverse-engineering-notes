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
    monsterTryActivateByDistance(record, (uint16_t)viewportIndex);

    if (save) {
        monsterSpawnFlagSet(save, game, typeId);
    }
    return slot;
}

void monsterGrantRewards(MonsterRewardStaging *staging, const uint8_t *record, uint8_t *globalFlags,
                          size_t globalFlagsSize) {
    bcd4Add(staging->gold, monsterLoot(record, MonsterLootGold));
    bcd4Add(staging->nuore, monsterLoot(record, MonsterLootNuore));
    bcd4Add(staging->ore, monsterLoot(record, MonsterLootOre));
    bcd4Add(staging->experience, monsterLoot(record, MonsterLootExperience));

    if (globalFlags) {
        globalFlagApplySigned(globalFlags, globalFlagsSize, (int16_t)monsterGetU16(record, MonsterFieldFlagOnDeath));
        globalFlagApplySigned(globalFlags, globalFlagsSize,
                               (int16_t)monsterGetU16(record, MonsterFieldFlagOnDeath2));
    }
}

void monsterPoolRemove(uint8_t *record, DungeonGrid *grid) {
    if (grid) {
        int worldX = monsterGetU16(record, MonsterFieldWorldX);
        int worldY = monsterGetU16(record, MonsterFieldWorldY);
        DungeonGridCell *cell = dungeonGridCellMutable(grid, worldY - grid->originRow, worldX - grid->originCol);
        if (cell) {
            cell->flags &= (uint16_t)~DungeonGridCellFlagOverlay;
            cell->reserved4 = 0;
        }
    }
    memset(record, 0, MonsterRecordSize);
}

MonsterObstacle monsterClassifyObstacle(GameKind game, uint16_t wallType, uint16_t floorType) {
    if (game == GameYendor3) {
        if ((wallType >= 2 && wallType <= 99) || (wallType >= 200 && wallType <= 299)) {
            return MonsterObstacleWall;
        }
        return floorType == 0 ? MonsterObstacleClear : MonsterObstacleFeature;
    }
    if (wallType >= 2 && wallType <= 15) {
        return MonsterObstacleWall;
    }
    if (floorType == 0) return MonsterObstacleClear;
    if (floorType <= 16) return MonsterObstacleFeature;
    if (floorType <= 20) return MonsterObstacleClear;
    if (floorType <= 62) return MonsterObstacleFeature;
    if (floorType <= 67) return MonsterObstacleClear;
    return MonsterObstacleFeature;
}

enum { MonsterApproachStepLimit = 5 };

void monsterApproachParty(uint8_t *record, GameKind game, const WorldMap *map, int partyWorldX, int partyWorldY,
                           RandomState *rng) {
    int worldX = monsterGetU16(record, MonsterFieldWorldX);
    int worldY = monsterGetU16(record, MonsterFieldWorldY);

    bool axisIsX;
    uint16_t directionBit;
    int step;
    int fixedCoord;
    int scanFrom, scanTo;

    if (worldY == partyWorldY) {
        axisIsX = true;
        fixedCoord = worldY;
        scanFrom = worldX;
        scanTo = partyWorldX;
        if (worldX < partyWorldX) {
            directionBit = MonsterWoundPartyMustFaceWest;
            step = 1;
        } else {
            directionBit = MonsterWoundPartyMustFaceEast;
            step = -1;
        }
    } else if (worldX == partyWorldX) {
        axisIsX = false;
        fixedCoord = worldX;
        scanFrom = worldY;
        scanTo = partyWorldY;
        if (worldY < partyWorldY) {
            directionBit = MonsterWoundPartyMustFaceNorth;
            step = 1;
        } else {
            directionBit = MonsterWoundPartyMustFaceSouth;
            step = -1;
        }
    } else {
        return; /* not aligned with the party on either axis */
    }

    uint16_t wound = monsterGetU16(record, MonsterFieldWound);
    wound &= (uint16_t)~(MonsterWoundPartyMustFaceNorth | MonsterWoundPartyMustFaceSouth |
                          MonsterWoundPartyMustFaceEast | MonsterWoundPartyMustFaceWest);
    wound |= directionBit;
    monsterSetU16(record, MonsterFieldWound, wound);

    int pos = scanFrom;
    for (int i = 0; i < MonsterApproachStepLimit; i++) {
        pos += step;
        if (pos == scanTo) {
            unsigned roll = randomInRange(rng, 100);
            unsigned threshold = monsterAmbushThreshold(monsterGetU16(record, MonsterFieldAwareness));
            if (roll <= threshold) {
                wound = monsterGetU16(record, MonsterFieldWound);
                wound |= MonsterWoundAmbushPending;
                monsterSetU16(record, MonsterFieldWound, wound);
            }
            return;
        }

        uint16_t wallType, floorType;
        if (axisIsX) {
            wallType = worldMapTileA(map, (unsigned)fixedCoord, (unsigned)pos);
            floorType = worldMapTileB(map, (unsigned)fixedCoord, (unsigned)pos);
        } else {
            wallType = worldMapTileA(map, (unsigned)pos, (unsigned)fixedCoord);
            floorType = worldMapTileB(map, (unsigned)pos, (unsigned)fixedCoord);
        }
        if (monsterClassifyObstacle(game, wallType, floorType) != MonsterObstacleClear) {
            return; /* blocked before reaching the party */
        }
    }
}

MonsterTurnOutcome monsterPoolProcessSlot(uint8_t *record, GameKind game, const WorldMap *map, DungeonGrid *grid,
                                           uint8_t *globalFlags, size_t globalFlagsSize,
                                           MonsterRewardStaging *staging, int partyWorldX, int partyWorldY,
                                           RandomState *rng) {
    if ((monsterGetU16(record, MonsterFieldState) & MonsterStateAware) == 0) {
        return MonsterTurnSkipped;
    }

    MonsterTickResult tick = monsterTickTimer(record);
    if (tick == MonsterTickOngoing) {
        return MonsterTurnSkipped;
    }
    if (tick == MonsterTickExpired) {
        monsterGrantRewards(staging, record, globalFlags, globalFlagsSize);
        monsterPoolRemove(record, grid);
        return MonsterTurnRemoved;
    }

    if (monsterGetU16(record, MonsterFieldApproachGate) == 0) {
        return MonsterTurnSkipped;
    }
    if (monsterGetU16(record, MonsterFieldState) & MonsterStateBusy) {
        return MonsterTurnSkipped;
    }

    monsterApproachParty(record, game, map, partyWorldX, partyWorldY, rng);
    return MonsterTurnApproached;
}

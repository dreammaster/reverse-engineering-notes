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

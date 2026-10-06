#include "windowbake.h"

#include <string.h>

#include "monsterpool.h"

WindowBake windowBakeForOutcome(InteractOutcome outcome, const WorldObjectRecord *object, uint16_t curgameValue) {
    WindowBake bake;
    memset(&bake, 0, sizeof(bake));
    switch (outcome) {
    case InteractOutcomeCurgameFlag40:
    case InteractOutcomeCurgameFallbackA:
        bake.flagsOr = 0x4000;
        break;
    case InteractOutcomeCurgameFallbackB:
        bake.flagsOr = 0x2000;
        break;
    case InteractOutcomeLockMagical:
        bake.flagsOr = 0x1000;
        break;
    case InteractOutcomeCurgameFlag10:
        bake.setFloor = true;
        bake.newType = curgameValue;
        break;
    case InteractOutcomeCurgameFlag8:
        bake.setWall = true;
        bake.newType = curgameValue;
        break;
    case InteractOutcomeUnspawnedMonster:
        bake.flagsOr = DungeonGridCellFlagOverlay;
        bake.monsterMarker = true;
        bake.monsterType = object ? object->value : 0;
        break;
    default:
        break;
    }
    return bake;
}

void windowBakeApply(DungeonGridCell *cell, const WindowBake *bake) {
    cell->flags |= bake->flagsOr;
    if (bake->setWall) {
        cell->wallType = bake->newType;
    }
    if (bake->setFloor) {
        cell->floorType = bake->newType;
    }
    if (bake->monsterMarker) {
        cell->reserved4 = bake->monsterType;
    }
}

unsigned dungeonGridBakeMarkers(DungeonGrid *grid, GameKind game, const WorldObjectTable *objects, const LockCatalog *locks, SaveGame *save) {
    unsigned changed = 0;
    for (int col = 0; col < DungeonGridSize; col++) {
        for (int row = 0; row < DungeonGridSize; row++) {
            WorldObjectRecord object;
            if (!worldObjectFind(objects, game, grid->originCol + col, grid->originRow + row, &object)) {
                continue;
            }
            LockRecord lock;
            memset(&lock, 0, sizeof(lock));
            uint16_t curgameFlags = 0, curgameValue = 0;
            bool alreadyDone = false, spawned = false;
            switch (interactSelectBranch(&object)) {
            case InteractBranchLock:
                lockCatalogRecord(locks, object.value, &lock);
                alreadyDone = interactBitmapTest(save, interactWorldObjectBitIndex(game, &object));
                break;
            case InteractBranchCurgame:
                lockCatalogCurgameRecord(locks, object.value, &curgameFlags, &curgameValue);
                alreadyDone = interactBitmapTest(save, interactWorldObjectBitIndex(game, &object));
                break;
            case InteractBranchMonsterSpawn:
                spawned = monsterSpawnFlagTest(save, game, object.value);
                break;
            default:
                break;
            }
            InteractOutcome outcome = interactClassify(&object, &lock, alreadyDone, curgameFlags, alreadyDone, spawned);
            WindowBake bake = windowBakeForOutcome(outcome, &object, curgameValue);
            DungeonGridCell *cell = dungeonGridCellMutable(grid, row, col);
            if (cell && (bake.flagsOr || bake.setWall || bake.setFloor || bake.monsterMarker)) {
                windowBakeApply(cell, &bake);
                changed++;
            }
        }
    }
    return changed;
}

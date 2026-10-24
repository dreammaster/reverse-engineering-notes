#include "windowbake.h"

#include <string.h>

#include "globalflags.h"
#include "monsterpool.h"

/* Chapter 3's object that only carries the story-event bit (flag 0x400 with none of the door, curgame, fixed-response, monster bits). */
static bool scriptedObject(GameKind game, const WorldObjectRecord *object) {
    return game == GameYendor3 && (object->flags & WorldObjectFlagStoryEvent) && !(object->flags & (WorldObjectFlagDoor | WorldObjectFlagCurgameRecord | WorldObjectFlagFixedResponse | WorldObjectFlagMonsterSpawn));
}

static bool questFlag(SaveGame *save, unsigned index) {
    return globalFlagTest(saveGlobalFlags(save), SaveHeaderGlobalFlagsSize, index);
}

bool windowBakeLocalMapFloor(GameKind game, const WorldObjectTable *objects, SaveGame *save, int x, int y, uint16_t *floorType) {
    WorldObjectRecord object;
    if (game != GameYendor3 || !worldObjectFind(objects, game, x, y, &object) || !scriptedObject(game, &object)) {
        return false;
    }
    switch (object.value) {
    case 1:
        *floorType = questFlag(save, 5) ? 0 : 0xD2;
        break;
    case 2:
        *floorType = questFlag(save, 0x1B) ? 0xDF : 0;
        break;
    case 3:
        *floorType = 0xE0;
        break;
    case 4:
        *floorType = 0xE7;
        break;
    case 5:
        *floorType = 0xD4;
        break;
    case 6:
        *floorType = 0xFA;
        break;
    default:
        *floorType = 0;
        break;
    }
    return true;
}

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

WindowBake windowBakeAtCell(GameKind game, const WorldObjectTable *objects, const LockCatalog *locks, SaveGame *save, int x, int y) {
    WorldObjectRecord object;
    if (!worldObjectFind(objects, game, x, y, &object)) {
        WindowBake none;
        memset(&none, 0, sizeof(none));
        return none;
    }
    if (scriptedObject(game, &object)) { /* ApplyScriptedMapCellOverrides */
        WindowBake scripted;
        memset(&scripted, 0, sizeof(scripted));
        if ((x == 40 && y == 62 && !questFlag(save, 5)) || (x == 69 && y == 50 && questFlag(save, 0x1B))) {
            scripted.flagsOr = 0x200;
            scripted.setFloor = true;
            scripted.newType = x == 40 ? 0xD2 : 0xDF;
        }
        return scripted;
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
    return windowBakeForOutcome(outcome, &object, curgameValue);
}

unsigned dungeonGridBakeMarkers(DungeonGrid *grid, GameKind game, const WorldObjectTable *objects, const LockCatalog *locks, SaveGame *save) {
    unsigned changed = 0;
    for (int col = 0; col < DungeonGridSize; col++) {
        for (int row = 0; row < DungeonGridSize; row++) {
            WindowBake bake = windowBakeAtCell(game, objects, locks, save, grid->originCol + col, grid->originRow + row);
            DungeonGridCell *cell = dungeonGridCellMutable(grid, row, col);
            if (cell && (bake.flagsOr || bake.setWall || bake.setFloor || bake.monsterMarker)) {
                windowBakeApply(cell, &bake);
                changed++;
            }
        }
    }
    return changed;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_windowbake test_windowbake.c ../windowbake.c ../interact.c ../monsterpool.c ../monster.c ../dungeongrid.c ../lockcatalog.c ../worldobjects.c \
 *       ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../globalflags.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c && ./test_windowbake
 *
 * The real-data checks read WORLD.DAT of yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "monsterpool.h"
#include "windowbake.h"
#include "worldmap_stdio.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void testOutcomes(void) {
    WorldObjectRecord marker = {10, WorldObjectFlagMonsterSpawn, 77};
    WindowBake b = windowBakeForOutcome(InteractOutcomeUnspawnedMonster, &marker, 0);
    check("an unspawned monster marks the cell with the type id", b.monsterMarker && b.monsterType == 77 && b.flagsOr == 0x400);
    b = windowBakeForOutcome(InteractOutcomeCurgameFlag40, NULL, 0);
    check("curgame flag 0x40 -> 0x4000", b.flagsOr == 0x4000 && !b.setWall && !b.setFloor);
    b = windowBakeForOutcome(InteractOutcomeCurgameFallbackA, NULL, 0);
    check("curgame fallback A -> 0x4000", b.flagsOr == 0x4000);
    b = windowBakeForOutcome(InteractOutcomeCurgameFallbackB, NULL, 0);
    check("curgame fallback B -> 0x2000", b.flagsOr == 0x2000);
    b = windowBakeForOutcome(InteractOutcomeLockMagical, NULL, 0);
    check("a magical lock -> 0x1000", b.flagsOr == 0x1000);
    b = windowBakeForOutcome(InteractOutcomeCurgameFlag10, NULL, 0x123);
    check("curgame flag 0x10 rewrites the floor type", b.setFloor && !b.setWall && b.newType == 0x123 && b.flagsOr == 0);
    b = windowBakeForOutcome(InteractOutcomeCurgameFlag8, NULL, 5);
    check("curgame flag 0x8 rewrites the wall type", b.setWall && !b.setFloor && b.newType == 5);
    WindowBake none[4] = {windowBakeForOutcome(InteractOutcomeNone, NULL, 9), windowBakeForOutcome(InteractOutcomeFixedResponse, NULL, 9),
                          windowBakeForOutcome(InteractOutcomeLockFlag40, NULL, 9), windowBakeForOutcome(InteractOutcomeLockPriced, NULL, 9)};
    bool nothing = true;
    for (int i = 0; i < 4; i++) {
        nothing = nothing && !none[i].flagsOr && !none[i].setWall && !none[i].setFloor && !none[i].monsterMarker;
    }
    check("outcomes 0, 4, 8 and 9 write nothing", nothing);

    DungeonGridCell cell = {1, 2, 3, 0x8000};
    b = windowBakeForOutcome(InteractOutcomeUnspawnedMonster, &marker, 0);
    windowBakeApply(&cell, &b);
    check("applying keeps the explored bit and the types, sets +4", cell.flags == 0x8400 && cell.reserved4 == 77 && cell.wallType == 1 && cell.floorType == 2);
    b = windowBakeForOutcome(InteractOutcomeCurgameFlag8, NULL, 5);
    windowBakeApply(&cell, &b);
    check("applying a wall rewrite", cell.wallType == 5 && cell.floorType == 2);
}

static void testCurgameRecords(void) {
    static LockCatalog catalog;
    static uint8_t region[LockBlock2RecordsYendor2 * LockRecordSize + LockCurgameCountYendor2 * 4];
    memset(region, 0, sizeof(region));
    size_t base = (size_t)LockRecordSize * 600;
    region[base + 4 * 6] = 0x09; /* id 7: flags 0x0009, value 5 */
    region[base + 4 * 6 + 2] = 5;
    region[base + 4 * 279] = 0x41; /* the last id of the block */
    check("a synthetic Chapter 2 region parses", lockCatalogParse(&catalog, GameYendor2, region, sizeof(region)));
    uint16_t flags = 0, value = 0;
    check("id 7 is read at block 2's size + 4 * 6", lockCatalogCurgameRecord(&catalog, 7, &flags, &value) && flags == 9 && value == 5);
    check("id 0 is refused", !lockCatalogCurgameRecord(&catalog, 0, &flags, &value));
    check("id 280 is the last, 281 is refused", lockCatalogCurgameRecord(&catalog, 280, &flags, &value) && flags == 0x41 && !lockCatalogCurgameRecord(&catalog, 281, &flags, &value));
    check("a region that stops after the locks has no curgame records", lockCatalogParse(&catalog, GameYendor2, region, LockRecordCountYendor2 * LockRecordSize) &&
                                                                       catalog.curgameCount == 0 && !lockCatalogCurgameRecord(&catalog, 1, &flags, &value));
}

static uint8_t *readFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *data = readFile(path, &size);
    if (!data) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    static WorldMap map;
    static WorldObjectTable objects;
    static LockCatalog locks;
    static DungeonGrid grid;
    static SaveGame save;
    bool parsed = worldMapParseWorldDat(&map, game, data, size) && worldObjectTableParseWorldDat(&objects, game, data, size) &&
                  lockCatalogParseWorldDat(&locks, game, data, size);
    check(label, parsed);
    if (!parsed) {
        free(data);
        return;
    }
    saveGameInit(&save, game);

    /* every curgame object of the whole table has a record in the catalog (Chapter 2: ids up to 52) */
    unsigned curgameObjects = 0, missing = 0;
    for (int col = 0x28; col < 0x28 + 800; col++) {
        for (int row = 0; row < 200; row++) {
            WorldObjectRecord object;
            if (worldObjectFind(&objects, game, col, row, &object) && interactSelectBranch(&object) == InteractBranchCurgame) {
                uint16_t f, v;
                curgameObjects++;
                missing += !lockCatalogCurgameRecord(&locks, object.value, &f, &v);
            }
        }
    }
    printf("     %s: %u curgame objects, %u without a record\n", label, curgameObjects, missing);
    check("every curgame object has a record in the catalog", curgameObjects > 0 && missing == 0);

    /* bake windows over the whole map and check the invariants of what was written */
    unsigned bakedCells = 0, monsterMarkers = 0, badMarkers = 0, windows = 0;
    for (int y = 20; y < 140; y += 30) {
        for (int x = 60; x < 780; x += 60) {
            dungeonGridBuild(&grid, game, &map, &save, x, y);
            unsigned before = 0;
            for (int r = 0; r < DungeonGridSize; r++) {
                for (int c = 0; c < DungeonGridSize; c++) {
                    before += (grid.cells[r][c].flags & 0x7C00) != 0;
                }
            }
            if (before != 0) {
                badMarkers++; /* the base copy never sets those bits */
            }
            bakedCells += dungeonGridBakeMarkers(&grid, game, &objects, &locks, &save);
            windows++;
            for (int r = 0; r < DungeonGridSize; r++) {
                for (int c = 0; c < DungeonGridSize; c++) {
                    const DungeonGridCell *cell = &grid.cells[r][c];
                    if (cell->flags & DungeonGridCellFlagOverlay) {
                        monsterMarkers++;
                        WorldObjectRecord object;
                        bool found = worldObjectFind(&objects, game, grid.originCol + c, grid.originRow + r, &object);
                        badMarkers += !(found && (object.flags & WorldObjectFlagMonsterSpawn) && cell->reserved4 == object.value);
                    }
                }
            }
        }
    }
    printf("     %s: %u windows, %u cells baked, %u monster markers\n", label, windows, bakedCells, monsterMarkers);
    check("windows bake something and every monster marker matches its world object", bakedCells > 0 && monsterMarkers > 0 && badMarkers == 0);

    /* once a monster's type is flagged as spawned its marker is not baked again; an unlocked bit silences a curgame cell */
    dungeonGridBuild(&grid, game, &map, &save, 60, 20);
    unsigned first = dungeonGridBakeMarkers(&grid, game, &objects, &locks, &save);
    for (int type = 0; type < 4096; type++) {
        monsterSpawnFlagSet(&save, game, (unsigned)type);
    }
    dungeonGridBuild(&grid, game, &map, &save, 60, 20);
    unsigned second = dungeonGridBakeMarkers(&grid, game, &objects, &locks, &save);
    check("with every monster type already spawned no monster marker is baked", second <= first);
    unsigned remaining = 0;
    for (int r = 0; r < DungeonGridSize; r++) {
        for (int c = 0; c < DungeonGridSize; c++) {
            remaining += (grid.cells[r][c].flags & DungeonGridCellFlagOverlay) != 0;
        }
    }
    check("... and none remains", remaining == 0);
    free(data);
}

int main(void) {
    testOutcomes();
    testCurgameRecords();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2 real data");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3 real data");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

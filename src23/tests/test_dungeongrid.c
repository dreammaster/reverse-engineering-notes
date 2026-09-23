/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_dungeongrid test_dungeongrid.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../savegame_stdio.c && ./test_dungeongrid
 *
 * Real-data checks read WORLD.DAT/CURGAME from yendor2/game and
 * yendor3/game (gitignored; skipped if absent). Set YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dungeongrid.h"
#include "movement.h"
#include "savegame.h"
#include "savegame_stdio.h"
#include "worldmap.h"
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

static void checkI32(const char *label, int actual, int expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %d, want %d\n", label, actual, expected);
    }
}

static void testOriginClamping(void) {
    int col, row;

    /*
     * Well inside the playable area: origin is just party position minus
     * 39, no clamping. (partyY=80: 80-39=41 is safely between rowMin=24
     * and rowMax-15=104.)
     */
    dungeonGridComputeOrigin(GameYendor2, 400, 80, &col, &row);
    checkI32("yendor2 centered: originCol = partyX - 39", col, 400 - 39);
    checkI32("yendor2 centered: originRow = partyY - 39", row, 80 - 39);

    /* Party right at the top-left corner of the playable box: clamp to boundsMin - 15. */
    dungeonGridComputeOrigin(GameYendor2, 0x28, 0x18, &col, &row);
    checkI32("yendor2 at top-left corner: originCol clamps to colMin-15", col, 0x28 - 15);
    checkI32("yendor2 at top-left corner: originRow clamps to rowMin-15", row, 0x18 - 15);

    /*
     * The bottom-right corner does NOT trigger the upper clamp: candidate
     * = boundsMax - 39 is always <= boundsMax - 15 (39 > 15), so for any
     * party position that's actually within the playable box, the upper
     * clamp can never fire -- it only exists for the (never legitimately
     * reached) case of a position past boundsMax, which we can still
     * exercise directly since this is a pure function with no bounds
     * validation of its own.
     */
    dungeonGridComputeOrigin(GameYendor2, 0x2F7, 0x77, &col, &row);
    checkI32("yendor2 at bottom-right corner: not clamped (39 > 15 makes the upper clamp unreachable in-bounds)",
             col, 0x2F7 - 39);
    checkI32("...same for row", row, 0x77 - 39);

    /* Exercise the upper clamp directly with an out-of-bounds position. */
    dungeonGridComputeOrigin(GameYendor2, 0x2F7 + 100, 0x18, &col, &row);
    checkI32("yendor2 past colMax: originCol clamps to colMax-15", col, 0x2F7 - 15);
    dungeonGridComputeOrigin(GameYendor2, 0x28, 0x77 + 100, &col, &row);
    checkI32("yendor2 past rowMax: originRow clamps to rowMax-15", row, 0x77 - 15);

    /* Chapter 3's taller map: same column bound, taller row bound (rowMax = 0x8F). */
    dungeonGridComputeOrigin(GameYendor3, 0x28, 0x8F + 100, &col, &row);
    checkI32("yendor3 past its own taller rowMax: clamps to rowMax-15", row, 0x8F - 15);

    /* Just below the clamp boundary: centered, not yet clamped. */
    dungeonGridComputeOrigin(GameYendor2, 0x28 + 39 + 1, 0x18, &col, &row);
    checkI32("one past the clamp point: centered, not clamped", col, 0x28 + 39 + 1 - 39);
}

static WorldMap g_map;
static uint8_t g_region[WorldMapRowsMax * WorldMapRowSize];

/* A synthetic map where wallType == row and floorType == col, for easy verification. */
static void buildSyntheticMap(void) {
    const WorldMapLayout *layout = worldMapLayout(GameYendor2);
    for (unsigned r = 0; r < layout->rowCount; r++) {
        for (unsigned c = 0; c < WorldMapColumns; c++) {
            uint8_t *cell = g_region + (size_t)r * WorldMapRowSize + (size_t)c * WorldMapColumnSize;
            cell[0] = (uint8_t)(r & 0xFF);
            cell[1] = (uint8_t)(r >> 8);
            cell[2] = (uint8_t)(c & 0xFF);
            cell[3] = (uint8_t)(c >> 8);
        }
    }
    check("synthetic map parses", worldMapParse(&g_map, GameYendor2, g_region, sizeof(g_region)));
}

static void testBuildSynthetic(void) {
    buildSyntheticMap();

    SaveGame save;
    saveGameInit(&save, GameYendor2);

    /* Mark a few specific (row, col) cells explored, MSB-first bit packing: byte col/8, bit (7 - col%8). */
    int partyX = 200, partyY = 60;
    int originCol, originRow;
    dungeonGridComputeOrigin(GameYendor2, partyX, partyY, &originCol, &originRow);

    int exploredRow = originRow + 5;
    int exploredCol = originCol + 3; /* bit 4 (7-3) of byte 0 */
    uint8_t *bitmapRow = saveGameRecord(&save, SaveSectionExploredMap, (unsigned)exploredRow);
    bitmapRow[exploredCol / 8] |= (uint8_t)(1u << (7 - (exploredCol % 8)));

    DungeonGrid grid;
    dungeonGridBuild(&grid, GameYendor2, &g_map, &save, partyX, partyY);

    checkI32("grid origin matches the standalone origin computation", grid.originCol, originCol);
    checkI32("grid origin row matches too", grid.originRow, originRow);

    const DungeonGridCell *cell = dungeonGridCell(&grid, 5, 3);
    check("cell (5,3) exists", cell != NULL);
    checkI32("cell (5,3) wallType == its world row", cell->wallType, exploredRow);
    checkI32("cell (5,3) floorType == its world col", cell->floorType, exploredCol);
    check("cell (5,3) is explored", dungeonGridCellIsExplored(cell));
    checkI32("cell (5,3) reserved4 is zeroed", cell->reserved4, 0);

    const DungeonGridCell *neighbor = dungeonGridCell(&grid, 5, 4);
    check("the adjacent unmarked cell is not explored", !dungeonGridCellIsExplored(neighbor));

    check("out-of-range row is NULL", dungeonGridCell(&grid, DungeonGridSize, 0) == NULL);
    check("out-of-range col is NULL", dungeonGridCell(&grid, 0, -1) == NULL);

    const DungeonGridCell *byWorldPos = dungeonGridCellAtWorldPos(&grid, exploredCol, exploredRow);
    check("world-position lookup finds the same cell", byWorldPos == cell);
    check("a world position outside the window is NULL",
          dungeonGridCellAtWorldPos(&grid, originCol - 1, originRow) == NULL);
}

static void testNullSave(void) {
    DungeonGrid grid;
    dungeonGridBuild(&grid, GameYendor2, &g_map, NULL, 200, 60);
    const DungeonGridCell *cell = dungeonGridCell(&grid, 5, 3);
    check("with no save, every cell reads as unexplored", !dungeonGridCellIsExplored(cell));
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir, WorldMap *outMap, SaveGame *outSave) {
    char path[512];
    const char *dir = getenv(envName);
    dir = dir ? dir : fallbackDir;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    if (!worldMapReadWorldDatFile(outMap, game, path)) {
        return false;
    }
    snprintf(path, sizeof(path), "%s/CURGAME", dir);
    return saveGameReadFile(outSave, path);
}

static void testRealYendor2(void) {
    static WorldMap map;
    static SaveGame save;
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", &map, &save)) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT/CURGAME)\n");
        g_skipCount++;
        return;
    }

    /* CURGAME's real saved position: world X=166, Y=36 (see savegame.h's header field docs). */
    int partyX = 166, partyY = 36;
    DungeonGrid grid;
    dungeonGridBuild(&grid, GameYendor2, &map, &save, partyX, partyY);

    check("the party's own cell is inside the built window",
          dungeonGridCellAtWorldPos(&grid, partyX, partyY) != NULL);

    const DungeonGridCell *own = dungeonGridCellAtWorldPos(&grid, partyX, partyY);
    checkI32("the party's own cell wallType matches the world map directly", own->wallType,
              worldMapTileA(&map, (unsigned)partyY, (unsigned)partyX));
    checkI32("...and its floorType too", own->floorType, worldMapTileB(&map, (unsigned)partyY, (unsigned)partyX));

    /* A freshly-started save should have at least some explored cells right around the party's start. */
    bool anyExplored = false;
    for (int r = 0; r < DungeonGridSize && !anyExplored; r++) {
        for (int c = 0; c < DungeonGridSize; c++) {
            if (dungeonGridCellIsExplored(&grid.cells[r][c])) {
                anyExplored = true;
                break;
            }
        }
    }
    check("at least one real cell near the party's start is marked explored", anyExplored);
}

static void testRealYendor3(void) {
    static WorldMap map;
    static SaveGame save;
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", &map, &save)) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT/CURGAME)\n");
        g_skipCount++;
        return;
    }
    uint16_t partyX = saveHeaderGetU16(&save, SaveHeaderWorldX);
    uint16_t partyY = saveHeaderGetU16(&save, SaveHeaderWorldY);
    check("yendor3's real saved position is within the playable bounds",
          movementInBounds(GameYendor3, partyX, partyY));

    DungeonGrid grid;
    dungeonGridBuild(&grid, GameYendor3, &map, &save, partyX, partyY);
    check("the party's own cell is inside the built window",
          dungeonGridCellAtWorldPos(&grid, partyX, partyY) != NULL);
}

int main(void) {
    testOriginClamping();
    testBuildSynthetic();
    testNullSave();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_worldmap test_worldmap.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_worldmap
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "savegame.h"
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

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static WorldMap g_map;
static uint8_t g_region[WorldMapRowsMax * WorldMapRowSize];

static void testLayouts(void) {
    checkU32("yendor2 has 144 rows", worldMapLayout(GameYendor2)->rowCount, WorldMapRowsYendor2);
    checkU32("yendor3 has 168 rows", worldMapLayout(GameYendor3)->rowCount, WorldMapRowsYendor3);
    checkU32("both start at WORLD.DAT offset 0", worldMapLayout(GameYendor2)->offset, 0);
    checkU32("row size is 800 columns x 4 bytes", WorldMapRowSize, 800 * 4);

    /* Cross-check against the savegame module: the explored-map bitmap is one row per map row. */
    checkU32("yendor2 row count matches the save's explored-map row count", WorldMapRowsYendor2,
             saveLayoutFor(GameYendor2)->sections[SaveSectionExploredMap].recordCount);
    checkU32("yendor3 row count matches the save's explored-map row count", WorldMapRowsYendor3,
             saveLayoutFor(GameYendor3)->sections[SaveSectionExploredMap].recordCount);
    checkU32("the explored-map row size is 800 columns / 8 bits per byte", WorldMapColumns / 8,
             saveLayoutFor(GameYendor2)->sections[SaveSectionExploredMap].recordSize);
}

static void testParse(void) {
    const WorldMapLayout *layout = worldMapLayout(GameYendor2);
    size_t needed = (size_t)layout->rowCount * WorldMapRowSize;
    for (size_t i = 0; i < needed; i++) {
        g_region[i] = (uint8_t)(i * 3 + 7);
    }
    check("a region one byte short is rejected", !worldMapParse(&g_map, GameYendor2, g_region, needed - 1));
    check("an exact region parses", worldMapParse(&g_map, GameYendor2, g_region, needed));
    check("row 0 col 0 is the region's first bytes", memcmp(worldMapCell(&g_map, 0, 0), g_region, 4) == 0);
    check("row 0 col 799 is the last column of row 0",
          memcmp(worldMapCell(&g_map, 0, 799), g_region + 799 * 4, 4) == 0);
    check("row 143 col 0 is the last row's first bytes",
          memcmp(worldMapCell(&g_map, 143, 0), g_region + 143 * WorldMapRowSize, 4) == 0);
    check("row 144 is out of range", worldMapCell(&g_map, 144, 0) == NULL);
    check("column 800 is out of range", worldMapCell(&g_map, 0, 800) == NULL);
    checkU32("tileA/tileB read little-endian from the cell",
             worldMapTileA(&g_map, 0, 0) | ((uint32_t)worldMapTileB(&g_map, 0, 0) << 16),
             (uint32_t)(g_region[0] | (g_region[1] << 8)) | ((uint32_t)(g_region[2] | (g_region[3] << 8)) << 16));
    checkU32("an out-of-range tileA reads as 0", worldMapTileA(&g_map, 200, 0), 0);

    static uint8_t world[0x71138 + 16];
    memcpy(world, g_region, needed);
    check("whole-file parse finds the region at offset 0",
          worldMapParseWorldDat(&g_map, GameYendor2, world, sizeof(world)) &&
              memcmp(worldMapCell(&g_map, 0, 0), g_region, 4) == 0);
    check("a WORLD.DAT shorter than the map region is rejected",
          !worldMapParseWorldDat(&g_map, GameYendor2, world, needed - 1));
}

static void testWallFloorTables(void) {
    uint16_t offset;
    check("wall type 0", worldMapWallPictureOffset(GameYendor2, 0, &offset) && offset == 0x16);
    check("wall type 10", worldMapWallPictureOffset(GameYendor2, 10, &offset) && offset == 0x50);
    check("wall type 57 (last)", worldMapWallPictureOffset(GameYendor2, 57, &offset) && offset == 0x10);
    check("wall type 58 is past the decoded range", !worldMapWallPictureOffset(GameYendor2, 58, &offset));

    check("floor type 0 is 0 (the common 'no overlay' case)",
          worldMapFloorPictureOffset(GameYendor2, 0, &offset) && offset == 0);
    check("floor type 17", worldMapFloorPictureOffset(GameYendor2, 17, &offset) && offset == 0x51);
    check("floor type 64 (last real entry)", worldMapFloorPictureOffset(GameYendor2, 64, &offset) && offset == 0x52);
    check("floor type 67 is a real, legitimately-zero slot",
          worldMapFloorPictureOffset(GameYendor2, 67, &offset) && offset == 0);
    check("floor type 68 is past the decoded range", !worldMapFloorPictureOffset(GameYendor2, 68, &offset));

    check("yendor3's tables aren't decoded (EMS-paged)", !worldMapWallPictureOffset(GameYendor3, 0, &offset));
    check("...same for the floor table", !worldMapFloorPictureOffset(GameYendor3, 0, &offset));
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return worldMapReadWorldDatFile(&g_map, game, path);
}

/*
 * The border-row pattern documented in worldmap.h: wall type (tileA)
 * alternates 0/1 column by column, floor type (tileB) stays 0.
 */
static bool isBorderRow(unsigned row) {
    for (unsigned c = 0; c < WorldMapColumns; c++) {
        uint16_t a = worldMapTileA(&g_map, row, c);
        uint16_t b = worldMapTileB(&g_map, row, c);
        if ((a != 0 && a != 1) || b != 0) {
            return false;
        }
    }
    return true;
}

static void checkInvariants(const char *name, GameKind game) {
    char label[96];
    bool boundedA = true;
    bool boundedB = true;
    unsigned maxA = 0;
    unsigned maxB = 0;
    unsigned touchedRows = 0;
    unsigned touchedCols = 0;
    bool rowTouched[WorldMapRowsMax] = {0};
    bool colTouched[WorldMapColumns] = {0};

    for (unsigned r = 0; r < g_map.rowCount; r++) {
        for (unsigned c = 0; c < WorldMapColumns; c++) {
            uint16_t a = worldMapTileA(&g_map, r, c);
            uint16_t b = worldMapTileB(&g_map, r, c);
            if (a > maxA) {
                maxA = a;
            }
            if (b > maxB) {
                maxB = b;
            }
            if (a != 0 || b != 0) {
                rowTouched[r] = true;
                colTouched[c] = true;
            }
        }
    }
    for (unsigned r = 0; r < g_map.rowCount; r++) {
        if (rowTouched[r]) {
            touchedRows++;
        }
    }
    for (unsigned c = 0; c < WorldMapColumns; c++) {
        if (colTouched[c]) {
            touchedCols++;
        }
    }
    /* Chapter 2's real data never exceeds table sizes (57/67); allow slack for Chapter 3's undecoded, larger tables. */
    if (game == GameYendor2) {
        boundedA = maxA < WorldMapWallTypeCountYendor2;
        boundedB = maxB < WorldMapFloorTypeCountYendor2;
    } else {
        boundedA = maxA < 4096;
        boundedB = maxB < 4096;
    }

    snprintf(label, sizeof(label), "%s: every row's data touches the full column range", name);
    check(label, touchedCols == WorldMapColumns);
    snprintf(label, sizeof(label), "%s: nearly every row has some real (nonzero) data", name);
    check(label, touchedRows + 8 >= g_map.rowCount);
    snprintf(label, sizeof(label), "%s: tile-A values stay in a sane bounded range", name);
    check(label, boundedA);
    snprintf(label, sizeof(label), "%s: tile-B values stay in a sane bounded range", name);
    check(label, boundedB);
    snprintf(label, sizeof(label), "%s: the first row is the border pattern", name);
    check(label, isBorderRow(0));
}

static void testRealYendor2(void) {
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game")) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    checkU32("parsed row count", g_map.rowCount, WorldMapRowsYendor2);

    /* CURGAME's real saved position: world X=166, Y=36 (both games' savegame docs). */
    uint16_t a = worldMapTileA(&g_map, 36, 166);
    uint16_t b = worldMapTileB(&g_map, 36, 166);
    check("the party's real saved position decodes to in-range tile types",
          a < WorldMapWallTypeCountYendor2 && b < WorldMapFloorTypeCountYendor2);

    checkInvariants("yendor2", GameYendor2);
}

static void testRealYendor3(void) {
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game")) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    checkU32("parsed row count", g_map.rowCount, WorldMapRowsYendor3);
    checkInvariants("yendor3", GameYendor3);
}

int main(void) {
    testLayouts();
    testParse();
    testWallFloorTables();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

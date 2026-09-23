/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_worldobjects test_worldobjects.c ../worldobjects.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_worldobjects
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "movement.h"
#include "worldobjects.h"

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

static void putU16(uint8_t *data, size_t offset, uint16_t value) {
    data[offset] = (uint8_t)(value & 0xFF);
    data[offset + 1] = (uint8_t)(value >> 8);
}

static uint8_t g_region[WorldObjectTableSize];
static WorldObjectTable g_table;

/*
 * Builds a synthetic table: every column points at a shared empty list
 * (just a 0xFFFF terminator) except a few columns with hand-placed
 * records, laid out right after the 720-entry (1440-byte) offset table.
 */
static void buildSyntheticTable(void) {
    memset(g_region, 0, sizeof(g_region));

    size_t p = WorldObjectColumns * 2;
    uint16_t emptyListOffset = (uint16_t)p;
    putU16(g_region, p, 0xFFFF);
    p += 2;

    /* Column 0 (worldX = 40 = colMin): two records, rows 30 and 50. */
    uint16_t col0Offset = (uint16_t)p;
    putU16(g_region, p, 30);
    putU16(g_region, p + 2, WorldObjectFlagDoor);
    putU16(g_region, p + 4, 5);
    p += 6;
    putU16(g_region, p, 50);
    putU16(g_region, p + 2, WorldObjectFlagMonsterSpawn);
    putU16(g_region, p + 4, 99);
    p += 6;
    putU16(g_region, p, 0xFFFF);
    p += 2;

    /* Column 1 (worldX = 41): a single record at rowMin (24), flag 0x2000 (untested by TryInteractAtPosition). */
    uint16_t col1Offset = (uint16_t)p;
    putU16(g_region, p, 0x18);
    putU16(g_region, p + 2, WorldObjectFlagUnknown2000);
    putU16(g_region, p + 4, 7);
    p += 6;
    putU16(g_region, p, 0xFFFF);
    p += 2;

    for (unsigned c = 0; c < WorldObjectColumns; c++) {
        uint16_t off = emptyListOffset;
        if (c == 0) off = col0Offset;
        if (c == 1) off = col1Offset;
        putU16(g_region, (size_t)c * 2, off);
    }

    check("synthetic table parses", worldObjectTableParse(&g_table, GameYendor2, g_region, sizeof(g_region)));
    check("a region one byte short is rejected",
          !worldObjectTableParse(&g_table, GameYendor2, g_region, sizeof(g_region) - 1));
}

static void testSyntheticFind(void) {
    WorldObjectRecord rec;

    check("finds the first record in column 0",
          worldObjectFind(&g_table, GameYendor2, 40, 30, &rec) && rec.y == 30 && rec.flags == WorldObjectFlagDoor &&
              rec.value == 5);
    check("finds the second record in column 0",
          worldObjectFind(&g_table, GameYendor2, 40, 50, &rec) && rec.flags == WorldObjectFlagMonsterSpawn &&
              rec.value == 99);
    check("a row between the two records is not found", !worldObjectFind(&g_table, GameYendor2, 40, 40, &rec));
    check("a row past the last record is not found", !worldObjectFind(&g_table, GameYendor2, 40, 60, &rec));
    check("a row before the first record is not found", !worldObjectFind(&g_table, GameYendor2, 40, 24, &rec));

    check("finds the 0x2000-flagged record in column 1",
          worldObjectFind(&g_table, GameYendor2, 41, 24, &rec) && rec.flags == WorldObjectFlagUnknown2000 &&
              rec.value == 7);

    check("an empty column reports not found", !worldObjectFind(&g_table, GameYendor2, 42, 24, &rec));

    check("a column just outside colMin is out of bounds", !worldObjectFind(&g_table, GameYendor2, 0x27, 24, &rec));
    check("a column just outside colMax is out of bounds", !worldObjectFind(&g_table, GameYendor2, 0x2F8, 24, &rec));
    check("a row just outside rowMin is out of bounds", !worldObjectFind(&g_table, GameYendor2, 40, 0x17, &rec));
    check("a row just outside rowMax is out of bounds", !worldObjectFind(&g_table, GameYendor2, 40, 0x78, &rec));
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir, WorldObjectTable *outTable) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return false;
    }
    uint8_t *buf = malloc((size_t)size);
    bool ok = buf && fread(buf, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    if (ok) {
        ok = worldObjectTableParseWorldDat(outTable, game, buf, (size_t)size);
    }
    free(buf);
    return ok;
}

typedef struct {
    unsigned total;
    unsigned doorCount, monsterSpawnCount, curgameRecordCount, fixedResponseCount, unknown2000Count, otherCount;
} FlagTally;

static FlagTally tallyAllRecords(const WorldObjectTable *table, GameKind game) {
    FlagTally tally = {0, 0, 0, 0, 0, 0, 0};
    const MovementBounds *b = movementBounds(game);
    for (unsigned col = b->colMin; col <= b->colMax; col++) {
        for (unsigned row = b->rowMin; row <= b->rowMax; row++) {
            WorldObjectRecord rec;
            if (!worldObjectFind(table, game, (int)col, (int)row, &rec)) {
                continue;
            }
            tally.total++;
            switch (rec.flags) {
            case WorldObjectFlagDoor: tally.doorCount++; break;
            case WorldObjectFlagMonsterSpawn: tally.monsterSpawnCount++; break;
            case WorldObjectFlagCurgameRecord: tally.curgameRecordCount++; break;
            case WorldObjectFlagFixedResponse: tally.fixedResponseCount++; break;
            case WorldObjectFlagUnknown2000: tally.unknown2000Count++; break;
            default: tally.otherCount++; break;
            }
        }
    }
    return tally;
}

static void testRealYendor2(void) {
    static WorldObjectTable table;
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", &table)) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    checkU32("yendor2 layout offset", worldObjectTableLayout(GameYendor2)->offset, 0x1A1141);

    WorldObjectRecord rec;
    check("yendor2 worldX=40's first reachable record is the door at row 119",
          worldObjectFind(&table, GameYendor2, 40, 119, &rec) && rec.flags == WorldObjectFlagDoor && rec.value == 1);

    FlagTally tally = tallyAllRecords(&table, GameYendor2);
    checkU32("yendor2 total reachable records", tally.total, 3108);
    checkU32("yendor2 monster-spawn markers (0x800)", tally.monsterSpawnCount, 2141);
    checkU32("yendor2 door/lock markers (0x8000)", tally.doorCount, 443);
    checkU32("yendor2 curgame-record markers (0x4000)", tally.curgameRecordCount, 231);
    checkU32("yendor2 unknown 0x2000 markers", tally.unknown2000Count, 187);
    checkU32("yendor2 fixed-response markers (0x1000)", tally.fixedResponseCount, 105);
    checkU32("yendor2 records with any other flag combination", tally.otherCount, 1);
}

static void testRealYendor3(void) {
    static WorldObjectTable table;
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", &table)) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    checkU32("yendor3 layout offset", worldObjectTableLayout(GameYendor3)->offset, 0x41090D);

    WorldObjectRecord rec;
    /* flags 0x400 is the rare, uninterpreted bit -- see WorldObjectFlagUnknown2000's doc comment for its cousin. */
    check("yendor3 worldX=40's first reachable record has the rare 0x400 flag at row 62",
          worldObjectFind(&table, GameYendor3, 40, 62, &rec) && rec.flags == 0x0400 && rec.value == 1);

    FlagTally tally = tallyAllRecords(&table, GameYendor3);
    checkU32("yendor3 total reachable records", tally.total, 2572);
    checkU32("yendor3 monster-spawn markers (0x800)", tally.monsterSpawnCount, 1862);
    checkU32("yendor3 door/lock markers (0x8000)", tally.doorCount, 355);
    checkU32("yendor3 fixed-response markers (0x1000)", tally.fixedResponseCount, 139);
    checkU32("yendor3 unknown 0x2000 markers", tally.unknown2000Count, 139);
    checkU32("yendor3 curgame-record markers (0x4000)", tally.curgameRecordCount, 71);
    checkU32("yendor3 records with any other flag combination", tally.otherCount, 6);
}

int main(void) {
    buildSyntheticTable();
    testSyntheticFind();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

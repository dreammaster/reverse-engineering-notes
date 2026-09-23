/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monsterpool test_monsterpool.c ../monsterpool.c ../dungeongrid.c ../movement.c ../monster.c ../monster_stdio.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c && ./test_monsterpool
 */
#include <stdio.h>
#include <string.h>

#include "dungeongrid.h"
#include "monster.h"
#include "monsterpool.h"
#include "random.h"
#include "savegame.h"

static int g_failureCount = 0;

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

static void testSpawnFlagBits(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    check("type id 0 starts clear", !monsterSpawnFlagTest(&save, GameYendor2, 0));
    monsterSpawnFlagSet(&save, GameYendor2, 0);
    check("type id 0 set", monsterSpawnFlagTest(&save, GameYendor2, 0));

    uint8_t *byte0 = saveGameRecord(&save, SaveSectionMonsterSpawnFlags, 0);
    checkU32("type id 0 sets the MSB of byte 0", *byte0, 0x80);

    monsterSpawnFlagSet(&save, GameYendor2, 7);
    checkU32("type id 7 sets the LSB of byte 0 (MSB-first packing)", *byte0, 0x81);

    monsterSpawnFlagSet(&save, GameYendor2, 8);
    uint8_t *byte1 = saveGameRecord(&save, SaveSectionMonsterSpawnFlags, 1);
    checkU32("type id 8 sets the MSB of byte 1, not byte 0", *byte1, 0x80);

    monsterSpawnFlagClear(&save, GameYendor2, 0);
    check("clearing type id 0 leaves type id 7 alone", monsterSpawnFlagTest(&save, GameYendor2, 7));
    check("type id 0 is clear again", !monsterSpawnFlagTest(&save, GameYendor2, 0));

    unsigned lastValid = saveLayoutFor(GameYendor2)->sections[SaveSectionMonsterSpawnFlags].recordCount * 8 - 1;
    monsterSpawnFlagSet(&save, GameYendor2, lastValid);
    check("the last valid type id round-trips", monsterSpawnFlagTest(&save, GameYendor2, lastValid));

    check("an out-of-range type id reads as clear, not a crash",
          !monsterSpawnFlagTest(&save, GameYendor2, lastValid + 8));
    monsterSpawnFlagSet(&save, GameYendor2, lastValid + 8); /* should be a safe no-op */
    check("...and setting one is a safe no-op", !monsterSpawnFlagTest(&save, GameYendor2, lastValid + 8));
}

static uint8_t g_pool[MonsterPoolSize * MonsterRecordSize];
static DungeonGrid g_grid;

static void placeMonster(unsigned slot, uint16_t type, uint16_t worldX, uint16_t worldY) {
    uint8_t *record = g_pool + (size_t)slot * MonsterRecordSize;
    memset(record, 0, MonsterRecordSize);
    monsterSetU16(record, MonsterFieldType, type);
    monsterSetU16(record, MonsterFieldWorldX, worldX);
    monsterSetU16(record, MonsterFieldWorldY, worldY);
}

static void testPoolRefresh(void) {
    memset(g_pool, 0, sizeof(g_pool));
    memset(&g_grid, 0, sizeof(g_grid));
    g_grid.game = GameYendor2;
    g_grid.originCol = 100;
    g_grid.originRow = 50;

    /* slot 0 left empty (type 0). */
    placeMonster(1, 42, 105, 55); /* well within the window */
    placeMonster(2, 43, 100 + DungeonGridSize, 50); /* the inclusive-edge case: in-window, but no grid cell */
    placeMonster(3, 44, 100 + DungeonGridSize + 1, 50); /* just past the edge: out of window */
    placeMonster(4, 45, 99, 50); /* one below originCol: out of window */

    SaveGame save;
    saveGameInit(&save, GameYendor2);
    monsterSpawnFlagSet(&save, GameYendor2, 44);
    monsterSpawnFlagSet(&save, GameYendor2, 45);

    unsigned stillOccupied = monsterPoolRefreshWindow(g_pool, &g_grid, &save);
    checkU32("2 of 4 occupied slots survive the refresh (slots 3 and 4 scrolled out)", stillOccupied, 2);

    uint8_t *rec1 = g_pool + 1 * MonsterRecordSize;
    checkU32("slot 1 keeps its type", monsterGetU16(rec1, MonsterFieldType), 42);
    checkU32("slot 1's cell offset is relative to the grid origin", monsterGetU16(rec1, MonsterFieldCell),
             (uint16_t)(5 * 0x270 + 5 * 8));
    const DungeonGridCell *cell1 = dungeonGridCell(&g_grid, 5, 5);
    checkU32("slot 1's overlay type id is baked into its grid cell", cell1->reserved4, 42);
    check("slot 1's grid cell gets the overlay flag", (cell1->flags & DungeonGridCellFlagOverlay) != 0);

    uint8_t *rec2 = g_pool + 2 * MonsterRecordSize;
    checkU32("slot 2 (edge case) survives even though it has no grid cell", monsterGetU16(rec2, MonsterFieldType),
             43);

    uint8_t *rec3 = g_pool + 3 * MonsterRecordSize;
    checkU32("slot 3 (just past the edge) is despawned", monsterGetU16(rec3, MonsterFieldType), 0);
    check("slot 3's spawn flag is cleared on despawn", !monsterSpawnFlagTest(&save, GameYendor2, 44));

    uint8_t *rec4 = g_pool + 4 * MonsterRecordSize;
    checkU32("slot 4 (below originCol) is despawned", monsterGetU16(rec4, MonsterFieldType), 0);
    check("slot 4's spawn flag is cleared on despawn", !monsterSpawnFlagTest(&save, GameYendor2, 45));

    check("an all-zero despawned record is fully zeroed, not just the type field",
          rec3[0] == 0 && rec3[MonsterRecordSize - 1] == 0);
}

static void testPoolRefreshWithoutSave(void) {
    memset(g_pool, 0, sizeof(g_pool));
    memset(&g_grid, 0, sizeof(g_grid));
    g_grid.game = GameYendor2;
    g_grid.originCol = 0;
    g_grid.originRow = 0;

    placeMonster(0, 7, 500, 500); /* far out of any reasonable window */
    unsigned stillOccupied = monsterPoolRefreshWindow(g_pool, &g_grid, NULL);
    checkU32("a NULL save still despawns out-of-window monsters", stillOccupied, 0);
    checkU32("the record is zeroed even without a save to update", monsterGetU16(g_pool, MonsterFieldType), 0);
}

static MonsterCatalog buildTestCatalog(void) {
    MonsterCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.blockCount = 2; /* 0 = empty, 1 = our one test monster */
    catalog.lookupCount = 10;

    uint8_t *block1 = catalog.blocks + MonsterBlockSize;
    /* Block offsets are record offsets minus MonsterBlockOffset. */
    block1[MonsterFieldSpriteBase - MonsterBlockOffset] = 100;
    block1[MonsterFieldMaxHealth - MonsterBlockOffset] = 50;

    catalog.lookup[5 * 2] = 1; /* type id 5 -> block 1 */
    catalog.lookup[5 * 2 + 1] = 0;
    return catalog;
}

static void testOffsetTable(void) {
    check("north table exists", monsterSpawnOffsetTable(SaveFacingNorth) != NULL);
    check("an invalid facing has no table", monsterSpawnOffsetTable(0) == NULL);

    /* Spot-check against the real extracted data (dump_spawn_offset_tables.py). */
    const MonsterSpawnOffset *north = monsterSpawnOffsetTable(SaveFacingNorth);
    check("north[0] is the far-left corner of the widest row", north[0].dx == -8 && north[0].dy == -6);
    check("north[50] (last entry) is directly ahead, one cell forward", north[50].dx == 1 && north[50].dy == 0);

    const MonsterSpawnOffset *east = monsterSpawnOffsetTable(SaveFacingEast);
    check("east[0] mirrors north[0] rotated 90 degrees", east[0].dx == 6 && east[0].dy == -8);
}

static void testPoolSpawn(void) {
    MonsterCatalog catalog = buildTestCatalog();
    memset(g_pool, 0, sizeof(g_pool));

    SaveGame save;
    saveGameInit(&save, GameYendor2);
    RandomState rng;
    randomStart(&rng, 30, 50);

    check("type id 5 starts unspawned", !monsterSpawnFlagTest(&save, GameYendor2, 5));

    int slot = monsterPoolSpawn(g_pool, &catalog, &save, GameYendor2, SaveFacingNorth, 200, 60, 150, 160,
                                 /*viewportIndex=*/50, /*typeId=*/5, &rng);
    check("spawn succeeds and returns a valid slot", slot >= 0 && slot < (int)MonsterPoolSize);

    uint8_t *record = g_pool + (size_t)slot * MonsterRecordSize;
    checkU32("spawned record has the right type", monsterGetU16(record, MonsterFieldType), 5);
    checkU32("spawned record's catalog fields copied through", monsterGetU16(record, MonsterFieldSpriteBase), 100);
    checkU32("spawned record's health starts at max", monsterGetU16(record, MonsterFieldHealth), 50);
    /* viewportIndex 50 is north[50] = (dx=1, dy=0); party at (200,60), facing north. */
    checkU32("spawned record's world position uses the offset table", monsterGetU16(record, MonsterFieldWorldX),
             201);
    checkU32("...and worldY too", monsterGetU16(record, MonsterFieldWorldY), 60);
    checkU32("spawned record's cell offset is relative to the given grid origin", monsterGetU16(record, MonsterFieldCell),
             (uint16_t)((60 - 150) * 0x270 + (201 - 160) * 8));
    check("animation start is within spriteBase + RandomInRange(5)",
          monsterGetU16(record, MonsterFieldAnim) >= 100 && monsterGetU16(record, MonsterFieldAnim) <= 105);

    check("type id 5 is marked spawned afterward", monsterSpawnFlagTest(&save, GameYendor2, 5));

    uint8_t fullPool[MonsterPoolSize * MonsterRecordSize];
    memset(fullPool, 0, sizeof(fullPool));
    for (unsigned i = 0; i < MonsterPoolSize; i++) {
        monsterSetU16(fullPool + (size_t)i * MonsterRecordSize, MonsterFieldType, 1);
    }
    check("spawning into a full pool fails",
          monsterPoolSpawn(fullPool, &catalog, NULL, GameYendor2, SaveFacingNorth, 200, 60, 150, 160, 0, 5, &rng) ==
              -1);

    check("an unknown type id fails without touching the pool",
          monsterPoolSpawn(g_pool, &catalog, NULL, GameYendor2, SaveFacingNorth, 200, 60, 150, 160, 0, 999, &rng) ==
              -1);

    check("an out-of-range viewport index fails",
          monsterPoolSpawn(g_pool, &catalog, NULL, GameYendor2, SaveFacingNorth, 200, 60, 150, 160,
                            MonsterSpawnOffsetCount, 5, &rng) == -1);

    check("a NULL save is safe (spawn flag simply isn't touched)",
          monsterPoolSpawn(g_pool, &catalog, NULL, GameYendor2, SaveFacingSouth, 200, 60, 150, 160, 0, 5, &rng) >=
              0);
}

int main(void) {
    testSpawnFlagBits();
    testPoolRefresh();
    testPoolRefreshWithoutSave();
    testOffsetTable();
    testPoolSpawn();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

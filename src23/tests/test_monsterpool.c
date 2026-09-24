/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monsterpool test_monsterpool.c ../monsterpool.c ../dungeongrid.c ../movement.c ../monster.c ../monster_stdio.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c ../bcd4.c ../globalflags.c ../party.c ../item.c && ./test_monsterpool
 */
#include <stdio.h>
#include <string.h>

#include "bcd4.h"
#include "dungeongrid.h"
#include "globalflags.h"
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

static uint32_t bcdHex(const uint8_t *value) {
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static void testGrantRewards(void) {
    uint8_t record[MonsterRecordSize];
    memset(record, 0, sizeof(record));
    bcd4FromU16((uint8_t *)(record + MonsterFieldLootGold), 495);
    bcd4FromU16((uint8_t *)(record + MonsterFieldLootNuore), 10);
    bcd4FromU16((uint8_t *)(record + MonsterFieldLootOre), 5);
    bcd4FromU16((uint8_t *)(record + MonsterFieldExperience), 1340);
    monsterSetU16(record, MonsterFieldFlagOnDeath, (uint16_t)14);   /* positive: set flag 14 */
    monsterSetU16(record, MonsterFieldFlagOnDeath2, (uint16_t)-3);  /* negative: clear flag 3 */

    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    /* Pre-seed staging.gold to check it accumulates (+=), not overwrites. */
    bcd4FromU16(staging.gold, 5);

    uint8_t flags[4] = {0};
    globalFlagSet(flags, sizeof(flags), 3); /* start set, so we can observe it being cleared */

    monsterGrantRewards(&staging, record, flags, sizeof(flags));

    checkU32("gold accumulates onto the pre-existing staging value", bcdHex(staging.gold), 0x00000500);
    checkU32("nuore staged", bcdHex(staging.nuore), 0x00000010);
    checkU32("ore staged", bcdHex(staging.ore), 0x00000005);
    checkU32("experience staged", bcdHex(staging.experience), 0x00001340);
    check("positive on-death flag (14) got set", globalFlagTest(flags, sizeof(flags), 14));
    check("negative on-death flag (-3) got cleared", !globalFlagTest(flags, sizeof(flags), 3));

    /* A NULL globalFlags buffer must not crash and must still stage loot. */
    MonsterRewardStaging staging2;
    memset(&staging2, 0, sizeof(staging2));
    monsterGrantRewards(&staging2, record, NULL, 0);
    checkU32("loot still stages with globalFlags == NULL", bcdHex(staging2.gold), 0x00000495);
}

static void testRewardsAward(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    /* Slot 0: id 1, plenty of room to level, not incapacitated. */
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
    uint8_t *member1 = saveGamePartyRecordById(&save, 1);
    partySetU16(member1, PartyFieldLevel, 1);

    /* Slot 1: id 2, incapacitated -- must not receive XP, but CheckForLevelUp still runs (a no-op for it). */
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 1 * 2, 2);
    uint8_t *member2 = saveGamePartyRecordById(&save, 2);
    partySetU16(member2, PartyFieldLevel, 1);
    partySetU16(member2, PartyFieldStatusFlags, PartyStatusStoned);

    /* Slots 2-3: empty. */
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 2 * 2, 0);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 3 * 2, 0);

    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    bcd4FromU16(staging.gold, 100);
    bcd4FromU16(staging.ore, 5);
    bcd4FromU16(staging.nuore, 3);
    bcd4FromU16(staging.experience, 700); /* crosses yendor2's 680 first-level threshold */

    monsterRewardsAward(&save, GameYendor2, &staging);

    checkU32("gold drains into SaveHeaderGold", bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x00000100);
    checkU32("ore drains into SaveHeaderOreCounter1 (not OreCounter2)",
             bcdHex(saveHeaderBcd4(&save, SaveHeaderOreCounter1)), 0x00000005);
    checkU32("nuore drains into SaveHeaderOreCounter2 (not OreCounter1)",
             bcdHex(saveHeaderBcd4(&save, SaveHeaderOreCounter2)), 0x00000003);

    checkU32("non-incapacitated member gains the staged experience", bcdHex(partyExperience(member1)), 0x00000700);
    check("non-incapacitated member levels up from the awarded experience",
          partyGetU16(member1, PartyFieldPendingLevel) == 2);

    checkU32("incapacitated member gains no experience", bcdHex(partyExperience(member2)), 0);
    check("incapacitated member has no pending level (its own CheckForLevelUp guard)",
          partyGetU16(member2, PartyFieldPendingLevel) == 0);

    /* A second award accumulates onto the same permanent counters. */
    monsterRewardsAward(&save, GameYendor2, &staging);
    checkU32("gold accumulates across two awards", bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x00000200);
}

static void testPoolRemove(void) {
    memset(&g_grid, 0, sizeof(g_grid));
    g_grid.game = GameYendor2;
    g_grid.originCol = 100;
    g_grid.originRow = 50;

    uint8_t record[MonsterRecordSize];
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldType, 7);
    monsterSetU16(record, MonsterFieldWorldX, 105);
    monsterSetU16(record, MonsterFieldWorldY, 55);

    DungeonGridCell *cell = dungeonGridCellMutable(&g_grid, 5, 5);
    cell->reserved4 = 7;
    cell->flags |= DungeonGridCellFlagOverlay;

    monsterPoolRemove(record, &g_grid);

    checkU32("removed record's type is zeroed", monsterGetU16(record, MonsterFieldType), 0);
    checkU32("removed record's world position is zeroed too", monsterGetU16(record, MonsterFieldWorldX), 0);
    check("the linked grid cell's overlay flag is cleared", (cell->flags & DungeonGridCellFlagOverlay) == 0);
    checkU32("the linked grid cell's overlay value is cleared", cell->reserved4, 0);

    /* A monster whose world position is no longer within the grid's window: no cell to touch, just despawn. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldType, 9);
    monsterSetU16(record, MonsterFieldWorldX, 5000);
    monsterSetU16(record, MonsterFieldWorldY, 5000);
    monsterPoolRemove(record, &g_grid);
    checkU32("a monster outside the window still despawns cleanly", monsterGetU16(record, MonsterFieldType), 0);

    /* A NULL grid must not crash. */
    monsterSetU16(record, MonsterFieldType, 3);
    monsterPoolRemove(record, NULL);
    checkU32("a NULL grid still despawns the record", monsterGetU16(record, MonsterFieldType), 0);
}

int main(void) {
    testSpawnFlagBits();
    testPoolRefresh();
    testPoolRefreshWithoutSave();
    testOffsetTable();
    testPoolSpawn();
    testGrantRewards();
    testRewardsAward();
    testPoolRemove();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

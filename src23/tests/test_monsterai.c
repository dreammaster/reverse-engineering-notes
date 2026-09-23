/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monsterai test_monsterai.c ../monsterpool.c ../monster.c ../monster_stdio.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c ../bcd4.c ../globalflags.c && ./test_monsterai
 */
#include <stdio.h>
#include <string.h>

#include "monster.h"
#include "monsterpool.h"
#include "random.h"
#include "worldmap.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void checkU32(const char *label, unsigned actual, unsigned expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static void testAmbushThreshold(void) {
    checkU32("no chance bits set -> lowest threshold (5)", monsterAmbushThreshold(0), 5);
    checkU32("Low -> 25", monsterAmbushThreshold(MonsterAmbushChanceLow), 25);
    checkU32("Medium -> 50", monsterAmbushThreshold(MonsterAmbushChanceMedium), 50);
    checkU32("High -> 75", monsterAmbushThreshold(MonsterAmbushChanceHigh), 75);
    checkU32("VeryHigh -> 90", monsterAmbushThreshold(MonsterAmbushChanceVeryHigh), 90);
    checkU32("VeryHigh wins when multiple bits are set", monsterAmbushThreshold(MonsterAmbushChanceVeryHigh | MonsterAmbushChanceLow),
             90);
    /* Awareness's OTHER bit range (0x20-0x100, "how far it notices") must not affect this. */
    checkU32("the distance-awareness bits don't affect the ambush threshold", monsterAmbushThreshold(MonsterAwarenessNear),
             5);
}

static void testClassifyObstacle(void) {
    /* Chapter 2: wall [2,15] blocked; floor {0}+[17,20]+[63,67] clear, else feature. */
    check("yendor2 wall 0 with floor 0 is clear", monsterClassifyObstacle(GameYendor2, 0, 0) == MonsterObstacleClear);
    check("yendor2 wall 2 is a wall regardless of floor",
          monsterClassifyObstacle(GameYendor2, 2, 0) == MonsterObstacleWall);
    check("yendor2 wall 15 is still a wall", monsterClassifyObstacle(GameYendor2, 15, 0) == MonsterObstacleWall);
    check("yendor2 wall 16 (past the wall band) with floor 1 is a feature",
          monsterClassifyObstacle(GameYendor2, 16, 1) == MonsterObstacleFeature);
    check("yendor2 wall 16 with floor 17 is clear", monsterClassifyObstacle(GameYendor2, 16, 17) == MonsterObstacleClear);
    check("yendor2 wall 16 with floor 20 is clear", monsterClassifyObstacle(GameYendor2, 16, 20) == MonsterObstacleClear);
    check("yendor2 wall 16 with floor 21 is a feature", monsterClassifyObstacle(GameYendor2, 16, 21) == MonsterObstacleFeature);
    check("yendor2 wall 16 with floor 62 is a feature", monsterClassifyObstacle(GameYendor2, 16, 62) == MonsterObstacleFeature);
    check("yendor2 wall 16 with floor 63 is clear", monsterClassifyObstacle(GameYendor2, 16, 63) == MonsterObstacleClear);
    check("yendor2 wall 16 with floor 67 is clear", monsterClassifyObstacle(GameYendor2, 16, 67) == MonsterObstacleClear);
    check("yendor2 wall 16 with floor 68 is a feature", monsterClassifyObstacle(GameYendor2, 16, 68) == MonsterObstacleFeature);

    /* Chapter 3: wall [2,99]u[200,299] blocked; floor 0 clear, nonzero feature. */
    check("yendor3 wall 0 floor 0 is clear", monsterClassifyObstacle(GameYendor3, 0, 0) == MonsterObstacleClear);
    check("yendor3 wall 99 is a wall", monsterClassifyObstacle(GameYendor3, 99, 0) == MonsterObstacleWall);
    check("yendor3 wall 100 is not a wall", monsterClassifyObstacle(GameYendor3, 100, 0) != MonsterObstacleWall);
    check("yendor3 wall 200 is a wall", monsterClassifyObstacle(GameYendor3, 200, 0) == MonsterObstacleWall);
    check("yendor3 wall 299 is a wall", monsterClassifyObstacle(GameYendor3, 299, 0) == MonsterObstacleWall);
    check("yendor3 wall 300 floor 1 is a feature", monsterClassifyObstacle(GameYendor3, 300, 1) == MonsterObstacleFeature);
}

static uint8_t g_region[WorldMapRowsMax * WorldMapRowSize];
static WorldMap g_map;

/* An all-clear map (wallType=100, floorType=0 everywhere -- normal, open floor for both games). */
static void buildOpenMap(GameKind game) {
    const WorldMapLayout *layout = worldMapLayout(game);
    for (unsigned r = 0; r < layout->rowCount; r++) {
        for (unsigned c = 0; c < WorldMapColumns; c++) {
            uint8_t *cell = g_region + (size_t)r * WorldMapRowSize + (size_t)c * WorldMapColumnSize;
            cell[0] = 100;
            cell[1] = 0;
            cell[2] = 0;
            cell[3] = 0;
        }
    }
    check("synthetic open map parses", worldMapParse(&g_map, game, g_region, (size_t)layout->rowCount * WorldMapRowSize));
}

static void putWall(unsigned row, unsigned col, uint16_t wallType) {
    uint8_t *cell = g_region + (size_t)row * WorldMapRowSize + (size_t)col * WorldMapColumnSize;
    cell[0] = (uint8_t)(wallType & 0xFF);
    cell[1] = (uint8_t)(wallType >> 8);
}

static uint8_t g_record[MonsterRecordSize];

static void setupMonster(unsigned worldX, unsigned worldY, uint16_t awareness) {
    memset(g_record, 0, sizeof(g_record));
    monsterSetU16(g_record, MonsterFieldType, 1);
    monsterSetU16(g_record, MonsterFieldWorldX, (uint16_t)worldX);
    monsterSetU16(g_record, MonsterFieldWorldY, (uint16_t)worldY);
    monsterSetU16(g_record, MonsterFieldAwareness, awareness);
}

static void testApproachAlignment(void) {
    buildOpenMap(GameYendor2);

    /* Not aligned with the party on either axis: nothing happens. */
    setupMonster(50, 50, 0);
    RandomState rng;
    randomStart(&rng, 12, 34);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    checkU32("unaligned monster gets no direction bit", monsterGetU16(g_record, MonsterFieldWound), 0);
}

static void testApproachDirections(void) {
    buildOpenMap(GameYendor2);
    RandomState rng;

    /* Monster north of the party (same column, smaller Y): party must face North. */
    setupMonster(60, 55, 0);
    randomStart(&rng, 1, 1);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("monster north of the party sets the 'face North' bit",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceNorth) != 0);

    /* Monster south of the party: party must face South. */
    setupMonster(60, 65, 0);
    randomStart(&rng, 1, 1);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("monster south of the party sets the 'face South' bit",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceSouth) != 0);

    /* Monster west of the party: party must face West. */
    setupMonster(55, 60, 0);
    randomStart(&rng, 1, 1);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("monster west of the party sets the 'face West' bit",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceWest) != 0);

    /* Monster east of the party: party must face East. */
    setupMonster(65, 60, 0);
    randomStart(&rng, 1, 1);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("monster east of the party sets the 'face East' bit",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceEast) != 0);
}

static void testApproachBlockedByWall(void) {
    buildOpenMap(GameYendor2);
    /* A wall directly between the party (row 60) and a monster 3 cells north (row 57). */
    putWall(58, 60, 10); /* row=58 (y), col=60 (x): wall type 10 is in [2,15], blocked */
    check("wall cell re-parses", worldMapParse(&g_map, GameYendor2, g_region,
                                                (size_t)worldMapLayout(GameYendor2)->rowCount * WorldMapRowSize));

    setupMonster(60, 57, 0); /* same column, path from row 58 onward passes through the wall */
    RandomState rng;
    randomStart(&rng, 5, 5);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("direction bit is still set even though the path is blocked",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceNorth) != 0);
    check("a blocked path never arms the ambush", (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundAmbushPending) == 0);
}

static void testApproachStepLimit(void) {
    buildOpenMap(GameYendor2);
    /* 6 cells away: outside the 5-step scan limit, even with a fully open path. */
    setupMonster(60, 54, 0);
    RandomState rng;
    randomStart(&rng, 7, 7);
    monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
    check("a monster more than 5 steps away never arms the ambush, even on a clear path",
          (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundAmbushPending) == 0);
}

static void testApproachAmbushRoll(void) {
    buildOpenMap(GameYendor2);
    /* Adjacent (1 step away), open path, and MonsterAmbushChanceVeryHigh (threshold 90): expect it to arm
       for almost any RNG seed. Not deterministic by construction, so just check it CAN succeed. */
    bool armed = false;
    for (uint8_t seed = 0; seed < 20 && !armed; seed++) {
        setupMonster(60, 59, (uint16_t)MonsterAmbushChanceVeryHigh);
        RandomState rng;
        randomStart(&rng, seed, seed);
        monsterApproachParty(g_record, GameYendor2, &g_map, 60, 60, &rng);
        armed = (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundAmbushPending) != 0;
    }
    check("an adjacent monster with a very-high ambush chance arms at least once across a handful of seeds", armed);
}

int main(void) {
    testAmbushThreshold();
    testClassifyObstacle();
    testApproachAlignment();
    testApproachDirections();
    testApproachBlockedByWall();
    testApproachStepLimit();
    testApproachAmbushRoll();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

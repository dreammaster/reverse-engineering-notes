/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monsterai test_monsterai.c ../monsterpool.c ../monster.c ../monster_stdio.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c ../bcd4.c ../globalflags.c ../party.c ../item.c ../effect.c && ./test_monsterai
 */
#include <stdio.h>
#include <string.h>

#include "monster.h"
#include "monsterpool.h"
#include "random.h"
#include "dungeongrid.h"
#include "movement.h"
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

static void testProcessSlotNotAware(void) {
    buildOpenMap(GameYendor2);
    setupMonster(60, 60, 0); /* MonsterFieldState defaults to 0 -> MonsterStateAware not set */
    RandomState rng;
    randomStart(&rng, 1, 1);
    MonsterTurnOutcome outcome =
        monsterPoolProcessSlot(g_record, GameYendor2, &g_map, NULL, NULL, 0, NULL, 60, 60, &rng);
    check("an unaware monster's turn is skipped entirely", outcome == MonsterTurnSkipped);
    checkU32("its tick fields are untouched", monsterGetU16(g_record, MonsterFieldHealth), 0);
}

static void testProcessSlotExpiresAndRemoves(void) {
    buildOpenMap(GameYendor2);
    DungeonGrid grid;
    memset(&grid, 0, sizeof(grid));
    grid.game = GameYendor2;
    grid.originCol = 55;
    grid.originRow = 55;

    setupMonster(60, 60, 0);
    monsterSetU16(g_record, MonsterFieldState, MonsterStateAware | 0x0400); /* aware + a tick gate bit */
    monsterSetU16(g_record, MonsterFieldHealth, 5);
    monsterSetU16(g_record, MonsterFieldTickAmount, 10);
    DungeonGridCell *cell = dungeonGridCellMutable(&grid, 60 - 55, 60 - 55);
    cell->reserved4 = 1;
    cell->flags |= DungeonGridCellFlagOverlay;

    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging)); /* loot fields are already zero from setupMonster's memset */

    RandomState rng;
    randomStart(&rng, 1, 1);
    MonsterTurnOutcome outcome =
        monsterPoolProcessSlot(g_record, GameYendor2, &g_map, &grid, NULL, 0, &staging, 60, 60, &rng);
    check("an expired tick removes the monster", outcome == MonsterTurnRemoved);
    checkU32("the record is zeroed", monsterGetU16(g_record, MonsterFieldType), 0);
    check("its grid cell overlay is cleared too", (cell->flags & DungeonGridCellFlagOverlay) == 0);
}

static void testProcessSlotOngoingIsSkipped(void) {
    buildOpenMap(GameYendor2);
    setupMonster(60, 55, 0); /* aligned north of the party -- would approach if given the chance */
    monsterSetU16(g_record, MonsterFieldState, MonsterStateAware | 0x3010); /* aware + double-decrement gate */
    monsterSetU16(g_record, MonsterFieldHealth, 100);
    monsterSetU16(g_record, MonsterFieldTickAmount, 10);
    monsterSetU16(g_record, MonsterFieldTickCountdown, 5);
    monsterSetU16(g_record, MonsterFieldApproachGate, 1);

    RandomState rng;
    randomStart(&rng, 1, 1);
    MonsterTurnOutcome outcome =
        monsterPoolProcessSlot(g_record, GameYendor2, &g_map, NULL, NULL, 0, NULL, 60, 60, &rng);
    check("a monster mid-tick (Ongoing) is skipped, never given an approach check", outcome == MonsterTurnSkipped);
    checkU32("it did not get a direction bit from an approach check", monsterGetU16(g_record, MonsterFieldWound), 0);
}

static void testProcessSlotApproachGates(void) {
    buildOpenMap(GameYendor2);

    /* Idle tick, but MonsterFieldApproachGate is 0: skipped, no approach. */
    setupMonster(60, 55, 0);
    monsterSetU16(g_record, MonsterFieldState, MonsterStateAware);
    monsterSetU16(g_record, MonsterFieldApproachGate, 0);
    RandomState rng;
    randomStart(&rng, 1, 1);
    MonsterTurnOutcome outcome =
        monsterPoolProcessSlot(g_record, GameYendor2, &g_map, NULL, NULL, 0, NULL, 60, 60, &rng);
    check("MonsterFieldApproachGate == 0 skips the approach check", outcome == MonsterTurnSkipped);

    /*
     * Idle tick, gate open, but MonsterStateBusy set: skipped.
     * MonsterStateBusy (0x800) happens to also be one of TickMonsterTimer's
     * own 0xFC10 gate bits, so a real tick runs here too -- give it a
     * harmless (zero-amount, non-expiring) tick setup so it can't
     * accidentally expire the monster and reach monsterGrantRewards with
     * no staging buffer.
     */
    setupMonster(60, 55, 0);
    monsterSetU16(g_record, MonsterFieldState, MonsterStateAware | MonsterStateBusy);
    monsterSetU16(g_record, MonsterFieldHealth, 100);
    monsterSetU16(g_record, MonsterFieldTickAmount, 0);
    monsterSetU16(g_record, MonsterFieldTickCountdown, 100);
    monsterSetU16(g_record, MonsterFieldApproachGate, 1);
    randomStart(&rng, 1, 1);
    outcome = monsterPoolProcessSlot(g_record, GameYendor2, &g_map, NULL, NULL, 0, NULL, 60, 60, &rng);
    check("MonsterStateBusy skips the approach check", outcome == MonsterTurnSkipped);

    /* Idle tick, gate open, not busy: the approach check runs. */
    setupMonster(60, 55, 0);
    monsterSetU16(g_record, MonsterFieldState, MonsterStateAware);
    monsterSetU16(g_record, MonsterFieldApproachGate, 1);
    randomStart(&rng, 1, 1);
    outcome = monsterPoolProcessSlot(g_record, GameYendor2, &g_map, NULL, NULL, 0, NULL, 60, 60, &rng);
    check("an aware, idle, gated-open, non-busy monster gets an approach check", outcome == MonsterTurnApproached);
    check("...and it set a direction bit", (monsterGetU16(g_record, MonsterFieldWound) & MonsterWoundPartyMustFaceNorth) != 0);
}

static DungeonGrid g_walkGrid;

static DungeonGridCell *walkCell(int x, int y) {
    return dungeonGridCellMutable(&g_walkGrid, y - g_walkGrid.originRow, x - g_walkGrid.originCol);
}

static void walkSetup(uint8_t *monster, int x, int y) {
    memset(&g_walkGrid, 0, sizeof(g_walkGrid));
    g_walkGrid.game = GameYendor2;
    g_walkGrid.originCol = 20;
    g_walkGrid.originRow = 20;
    for (int r = 0; r < DungeonGridSize; r++) {
        for (int c = 0; c < DungeonGridSize; c++) {
            g_walkGrid.cells[r][c].wallType = 20; /* ordinary walkable ground */
        }
    }
    memset(monster, 0, MonsterRecordSize);
    monsterSetU16(monster, MonsterFieldType, 7);
    monsterSetU16(monster, MonsterFieldWorldX, (uint16_t)x);
    monsterSetU16(monster, MonsterFieldWorldY, (uint16_t)y);
    monsterSetU16(monster, MonsterFieldWound, 0x1F00 | 0x8000);
    walkCell(x, y)->flags = 0x0400;
    walkCell(x, y)->reserved4 = 7;
}

static void testStepBlocked(void) {
    DungeonGridCell cell;
    bool hops;
    memset(&cell, 0, sizeof(cell));
    cell.wallType = 20;
    check("open ground is free", !monsterStepBlocked(GameYendor2, &cell, 0, &hops) && !hops);
    cell.flags = 0x0400;
    check("a cell with another monster (0x400) blocks, whatever the traits", monsterStepBlocked(GameYendor2, &cell, 0xFFFF, &hops));
    cell.flags = 0x2000;
    check("a door blocks without trait 0x10", monsterStepBlocked(GameYendor2, &cell, 0, &hops));
    check("...and is hopped (two cells) with it", !monsterStepBlocked(GameYendor2, &cell, 0x10, &hops) && hops);
    cell.flags = 0;
    cell.wallType = 8; /* the special wall range 6-11 */
    check("the special wall range blocks without 0x14", monsterStepBlocked(GameYendor2, &cell, 0, &hops));
    check("...and is hopped with 0x04", !monsterStepBlocked(GameYendor2, &cell, 0x04, &hops) && hops);
    cell.wallType = 1;
    check("water (wall type 0-1) blocks walkers and lets 0x08/0x02/0x10 traits through", monsterStepBlocked(GameYendor2, &cell, 0, &hops) &&
                                                                                           !monsterStepBlocked(GameYendor2, &cell, 0x02, &hops));
    cell.wallType = 3;
    check("a plain wall blocks", monsterStepBlocked(GameYendor2, &cell, 0, &hops));
    cell.wallType = 20;
    cell.floorType = 0x28;
    check("floor types 0x27-0x2A block even a 0x10 monster", monsterStepBlocked(GameYendor2, &cell, 0x10, &hops));
    cell.floorType = 0x25;
    check("floor type 0x25 needs trait 8", monsterStepBlocked(GameYendor2, &cell, 0, &hops) && !monsterStepBlocked(GameYendor2, &cell, 8, &hops));
    cell.wallType = 300;
    cell.floorType = 0;
    check("Chapter 3: trait 2 forbids ordinary terrain", monsterStepBlocked(GameYendor3, &cell, 2, &hops) && !monsterStepBlocked(GameYendor3, &cell, 0, &hops));
}

static void testWalk(void) {
    uint8_t m[MonsterRecordSize];
    walkSetup(m, 30, 30);
    check("a monster two rows away on another column steps horizontally toward the party",
          monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 35, 33) == MonsterMoveStepped && monsterGetU16(m, MonsterFieldWorldX) == 31 &&
              monsterGetU16(m, MonsterFieldWorldY) == 30);
    check("...the direction bits are cleared (0x8000 stays)", monsterGetU16(m, MonsterFieldWound) == 0x8000);
    check("...the markers moved with it", !(walkCell(30, 30)->flags & 0x400) && walkCell(30, 30)->reserved4 == 0 && (walkCell(31, 30)->flags & 0x400) &&
                                              walkCell(31, 30)->reserved4 == 7);
    check("...and its cell offset follows ((y - row) * 0x270 + (x - col) * 8)", monsterGetU16(m, MonsterFieldCell) == 10 * 0x270 + 11 * 8);

    walkSetup(m, 30, 30);
    monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 30, 36);
    check("in the party's column it steps along the column", monsterGetU16(m, MonsterFieldWorldX) == 30 && monsterGetU16(m, MonsterFieldWorldY) == 31);
    walkSetup(m, 30, 30);
    monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 30, 20);
    check("...upward when the party is above", monsterGetU16(m, MonsterFieldWorldY) == 29);

    walkSetup(m, 30, 30);
    walkCell(29, 30)->wallType = 3;
    check("a blocked horizontal step falls back to the vertical one", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 25, 35) == MonsterMoveStepped &&
                                                                          monsterGetU16(m, MonsterFieldWorldX) == 30 && monsterGetU16(m, MonsterFieldWorldY) == 31);
    walkSetup(m, 30, 30);
    walkCell(29, 30)->wallType = 3;
    check("...but not when already in the party's row", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 25, 30) == MonsterMoveNone &&
                                                             monsterGetU16(m, MonsterFieldWorldX) == 30);

    walkSetup(m, 30, 30);
    check("one row from the party it tries the vertical step first", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 36, 31) == MonsterMoveStepped &&
                                                                         monsterGetU16(m, MonsterFieldWorldY) == 31 && monsterGetU16(m, MonsterFieldWorldX) == 30);
    walkSetup(m, 30, 30);
    walkCell(30, 31)->wallType = 3;
    check("...falling back to the horizontal one", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 36, 31) == MonsterMoveStepped &&
                                                       monsterGetU16(m, MonsterFieldWorldX) == 31 && monsterGetU16(m, MonsterFieldWorldY) == 30);

    walkSetup(m, 30, 30);
    check("a step onto the party's cell is combat, nothing moves", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 31, 30) == MonsterMoveEngaged &&
                                                                       monsterGetU16(m, MonsterFieldWorldX) == 30 && (walkCell(30, 30)->flags & 0x400));
    walkSetup(m, 30, 30);
    check("on the party's own cell nothing happens", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 30, 30) == MonsterMoveNone);

    walkSetup(m, 30, 30);
    walkCell(31, 30)->flags = 0x2000;
    monsterSetU16(m, MonsterFieldAwareness, 0x10);
    check("a door-passing monster covers two cells and lands on the party's cell as combat", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 32, 30) == MonsterMoveEngaged);
    walkSetup(m, 30, 30);
    walkCell(31, 30)->flags = 0x2000;
    monsterSetU16(m, MonsterFieldAwareness, 0x10);
    check("...or past the party's column with the hop", monsterWalkTowardParty(m, GameYendor2, &g_walkGrid, 40, 30) == MonsterMoveStepped && monsterGetU16(m, MonsterFieldWorldX) == 32);
}

static void testFullTurn(void) {
    uint8_t m[MonsterRecordSize];
    RandomState rng;
    randomStart(&rng, 1, 1);
    walkSetup(m, 30, 30);
    MonsterFullTurn t = monsterPoolTakeTurn(m, GameYendor2, &g_map, &g_walkGrid, NULL, 0, NULL, 36, 36, &rng);
    check("an unaware monster neither approaches nor walks", t.turn == MonsterTurnSkipped && t.move == MonsterMoveNone && monsterGetU16(m, MonsterFieldWorldX) == 30);
    walkSetup(m, 30, 30);
    monsterSetU16(m, MonsterFieldState, MonsterStateAware);
    monsterSetU16(m, MonsterFieldApproachGate, 0);
    t = monsterPoolTakeTurn(m, GameYendor2, &g_map, &g_walkGrid, NULL, 0, NULL, 36, 36, &rng);
    check("with the approach gate closed the monster still walks toward the party", t.turn == MonsterTurnSkipped && t.move == MonsterMoveStepped &&
                                                                                    monsterGetU16(m, MonsterFieldWorldX) == 31);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    testAmbushThreshold();
    testClassifyObstacle();
    testApproachAlignment();
    testApproachDirections();
    testApproachBlockedByWall();
    testApproachStepLimit();
    testApproachAmbushRoll();
    testProcessSlotNotAware();
    testProcessSlotExpiresAndRemoves();
    testProcessSlotOngoingIsSkipped();
    testProcessSlotApproachGates();
    testStepBlocked();
    testWalk();
    testFullTurn();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

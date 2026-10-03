/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_spelljump test_spelljump.c ../spelljump.c ../spellrecord.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_spelljump
 */
#include <stdio.h>
#include <string.h>

#include "movement.h"
#include "savegame.h"
#include "spelljump.h"
#include "spellrecord.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

#define GRID 12

typedef struct {
    uint16_t wall[GRID][GRID];
    uint16_t floor[GRID][GRID];
} TestGrid;

static bool cellAt(void *context, int col, int row, uint16_t *wall, uint16_t *floor) {
    const TestGrid *grid = context;
    if (col < 0 || row < 0 || col >= GRID || row >= GRID) {
        return false;
    }
    *wall = grid->wall[row][col];
    *floor = grid->floor[row][col];
    return true;
}

static uint16_t g_normalWall, g_blockedWall, g_openFloor, g_impassableFloor;

/* Probes the real classifiers for representative values instead of hard-coding the games' thresholds. */
static void findRepresentatives(GameKind game) {
    g_normalWall = g_blockedWall = g_openFloor = g_impassableFloor = 0xFFFF;
    for (uint16_t v = 2; v < 400; v++) {
        MovementFloorClass c = movementClassifyFloorType(game, v);
        if (c == MovementFloorNormal && g_normalWall == 0xFFFF) {
            g_normalWall = v;
        }
        if (c == MovementFloorBlocked && g_blockedWall == 0xFFFF) {
            g_blockedWall = v;
        }
    }
    for (uint16_t v = 0; v < 400; v++) {
        bool imp = movementIsFloorTypeImpassable(game, v);
        if (!imp && g_openFloor == 0xFFFF) {
            g_openFloor = v;
        }
        if (imp && g_impassableFloor == 0xFFFF) {
            g_impassableFloor = v;
        }
    }
}

static void openGrid(TestGrid *grid) {
    for (int r = 0; r < GRID; r++) {
        for (int c = 0; c < GRID; c++) {
            grid->wall[r][c] = g_normalWall;
            grid->floor[r][c] = g_openFloor;
        }
    }
}

static void setJump(uint8_t *record, unsigned field, uint16_t value) {
    memset(record, 0, SpellRecordSize);
    record[field] = (uint8_t)(value & 0xFF);
    record[field + 1] = (uint8_t)(value >> 8);
}

static void testRepresentativesExist(GameKind game, const char *name) {
    findRepresentatives(game);
    char label[64];
    snprintf(label, sizeof(label), "%s: found normal/blocked wall and open/impassable floor types", name);
    check(label, g_normalWall != 0xFFFF && g_blockedWall != 0xFFFF && g_openFloor != 0xFFFF && g_impassableFloor != 0xFFFF);
}

static void testForwardWalkInEachFacing(GameKind game) {
    TestGrid grid;
    uint8_t rec[SpellRecordSize];
    openGrid(&grid);
    setJump(rec, SpellFieldJumpForward, 2);

    SpellJumpResult r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("forward 2 facing north lands 2 rows up", r.ok && r.col == 5 && r.row == 3);
    r = spellResolveJump(game, rec, SaveFacingEast, 5, 5, cellAt, &grid);
    check("facing east: 2 columns right", r.ok && r.col == 7 && r.row == 5);
    r = spellResolveJump(game, rec, SaveFacingSouth, 5, 5, cellAt, &grid);
    check("facing south: 2 rows down", r.ok && r.col == 5 && r.row == 7);
    r = spellResolveJump(game, rec, SaveFacingWest, 5, 5, cellAt, &grid);
    check("facing west: 2 columns left", r.ok && r.col == 3 && r.row == 5);
}

static void testRelativeDirections(GameKind game) {
    TestGrid grid;
    uint8_t rec[SpellRecordSize];
    openGrid(&grid);

    setJump(rec, SpellFieldJumpBackward, 1);
    SpellJumpResult r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("backward facing north is row+1", r.ok && r.col == 5 && r.row == 6);
    setJump(rec, SpellFieldJumpLeft, 1);
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("left facing north is col-1", r.ok && r.col == 4 && r.row == 5);
    r = spellResolveJump(game, rec, SaveFacingEast, 5, 5, cellAt, &grid);
    check("left facing east is row-1 (north)", r.ok && r.col == 5 && r.row == 4);
    setJump(rec, SpellFieldJumpRight, 1);
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("right facing north is col+1", r.ok && r.col == 6 && r.row == 5);
    r = spellResolveJump(game, rec, SaveFacingWest, 5, 5, cellAt, &grid);
    check("right facing west is row-1 (north)", r.ok && r.col == 5 && r.row == 4);
}

static void testWalkFailsOnBlockedCells(GameKind game) {
    TestGrid grid;
    uint8_t rec[SpellRecordSize];
    openGrid(&grid);
    setJump(rec, SpellFieldJumpForward, 2);

    grid.wall[4][5] = g_blockedWall; /* the intermediate cell north of (5,5) */
    SpellJumpResult r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("a blocked intermediate wall type fails the walk", !r.ok && r.col == 5 && r.row == 5);

    grid.wall[4][5] = 0; /* border-band type: skips the classification on an intermediate cell */
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("an intermediate wall type of 0 is skipped (walked over)", r.ok && r.row == 3);

    grid.wall[3][5] = 0;
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("but the final cell is always classified, so landing on type 0 fails",
          movementClassifyFloorType(game, 0) == MovementFloorNormal ? r.ok : !r.ok);

    openGrid(&grid);
    grid.floor[4][5] = g_impassableFloor;
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("an impassable floor on any cell, even intermediate, fails", !r.ok);

    openGrid(&grid);
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 1, cellAt, &grid);
    check("walking off the map fails", !r.ok);
}

static void testThroughIgnoresWhatLiesBetween(GameKind game) {
    TestGrid grid;
    uint8_t rec[SpellRecordSize];
    openGrid(&grid);
    setJump(rec, SpellFieldJumpThrough, 2);
    grid.wall[4][5] = g_blockedWall;
    grid.floor[4][5] = g_impassableFloor;

    SpellJumpResult r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("a wall in the way doesn't stop JUMP THROUGH", r.ok && r.col == 5 && r.row == 3);

    grid.wall[3][5] = g_blockedWall;
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("but a blocked destination does", !r.ok);

    openGrid(&grid);
    grid.floor[3][5] = g_impassableFloor;
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("...as does an impassable destination floor", !r.ok);
}

static void testPriorityAndEmptyRecord(GameKind game) {
    TestGrid grid;
    uint8_t rec[SpellRecordSize];
    openGrid(&grid);

    memset(rec, 0, sizeof(rec));
    check("no jump words at all fails", !spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid).ok);

    rec[SpellFieldJumpBackward] = 1;
    rec[SpellFieldJumpForward] = 3;
    SpellJumpResult r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("forward outranks backward", r.ok && r.row == 2);

    memset(rec, 0, sizeof(rec));
    rec[SpellFieldJumpRight] = 1;
    rec[SpellFieldJumpThrough] = 4;
    r = spellResolveJump(game, rec, SaveFacingNorth, 5, 5, cellAt, &grid);
    check("a walk word outranks through", r.ok && r.col == 6 && r.row == 5);
}

static void testCreatedItem(void) {
    uint8_t rec[SpellRecordSize];
    memset(rec, 0, sizeof(rec));
    rec[SpellFieldAttackMagnitude] = 55;
    rec[SpellFieldCreatedItemExtra] = 3;
    check("a fixed item needs no roll", spellCreatedItemRollBound(rec) == 0);
    SpellCreatedItem item = spellCreatedItem(rec, 99);
    check("the fixed id wins over any roll", item.itemId == 55);
    check("the extra word comes from 0x30", item.extra == 3);

    memset(rec, 0, sizeof(rec));
    rec[SpellFieldCreatedItemMin] = 0x32;
    rec[SpellFieldCreatedItemMax] = 0x36;
    check("random range bound is max - min", spellCreatedItemRollBound(rec) == 4);
    check("the roll is added to min", spellCreatedItem(rec, 2).itemId == 0x34);
}

int main(void) {
    GameKind games[2] = {GameYendor2, GameYendor3};
    const char *names[2] = {"yendor2", "yendor3"};
    for (int i = 0; i < 2; i++) {
        testRepresentativesExist(games[i], names[i]);
        testForwardWalkInEachFacing(games[i]);
        testRelativeDirections(games[i]);
        testWalkFailsOnBlockedCells(games[i]);
        testThroughIgnoresWhatLiesBetween(games[i]);
        testPriorityAndEmptyRecord(games[i]);
    }
    testCreatedItem();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_movement test_movement.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_movement
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>

#include "movement.h"
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

static const char *facingName(uint16_t facing) {
    switch (facing) {
        case SaveFacingNorth: return "N";
        case SaveFacingSouth: return "S";
        case SaveFacingEast: return "E";
        case SaveFacingWest: return "W";
        default: return "?";
    }
}

static void checkFacing(const char *label, uint16_t actual, uint16_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %s, want %s\n", label, facingName(actual), facingName(expected));
    }
}

/* Turning is a pure facing rotation; a full 4x turn-left or turn-right cycle returns to start. */
static void testTurning(void) {
    checkFacing("turn left from N", movementApply(MovementTurnLeft, SaveFacingNorth).facing, SaveFacingWest);
    checkFacing("turn left from W", movementApply(MovementTurnLeft, SaveFacingWest).facing, SaveFacingSouth);
    checkFacing("turn left from S", movementApply(MovementTurnLeft, SaveFacingSouth).facing, SaveFacingEast);
    checkFacing("turn left from E", movementApply(MovementTurnLeft, SaveFacingEast).facing, SaveFacingNorth);

    checkFacing("turn right from N", movementApply(MovementTurnRight, SaveFacingNorth).facing, SaveFacingEast);
    checkFacing("turn right from E", movementApply(MovementTurnRight, SaveFacingEast).facing, SaveFacingSouth);
    checkFacing("turn right from S", movementApply(MovementTurnRight, SaveFacingSouth).facing, SaveFacingWest);
    checkFacing("turn right from W", movementApply(MovementTurnRight, SaveFacingWest).facing, SaveFacingNorth);

    uint16_t facings[] = {SaveFacingNorth, SaveFacingSouth, SaveFacingEast, SaveFacingWest};
    for (size_t i = 0; i < 4; i++) {
        uint16_t f = facings[i];
        for (int n = 0; n < 4; n++) {
            f = movementApply(MovementTurnLeft, f).facing;
        }
        checkFacing("4 turn-lefts is a no-op", f, facings[i]);
    }

    check("turning never moves", movementApply(MovementTurnLeft, SaveFacingNorth).deltaCol == 0 &&
                                       movementApply(MovementTurnLeft, SaveFacingNorth).deltaRow == 0);
}

typedef struct {
    MovementAction action;
    uint16_t facing;
    int8_t deltaCol, deltaRow;
} MoveCase;

/* Forward/backward move along facing; strafing moves perpendicular. World Y grows southward (row index). */
static void testMoveDeltas(void) {
    static const MoveCase cases[] = {
        {MovementForward, SaveFacingNorth, 0, -1},  {MovementForward, SaveFacingSouth, 0, 1},
        {MovementForward, SaveFacingEast, 1, 0},    {MovementForward, SaveFacingWest, -1, 0},

        {MovementBackward, SaveFacingNorth, 0, 1},  {MovementBackward, SaveFacingSouth, 0, -1},
        {MovementBackward, SaveFacingEast, -1, 0},  {MovementBackward, SaveFacingWest, 1, 0},

        {MovementStrafeLeft, SaveFacingNorth, -1, 0}, {MovementStrafeLeft, SaveFacingSouth, 1, 0},
        {MovementStrafeLeft, SaveFacingEast, 0, -1},  {MovementStrafeLeft, SaveFacingWest, 0, 1},

        {MovementStrafeRight, SaveFacingNorth, 1, 0}, {MovementStrafeRight, SaveFacingSouth, -1, 0},
        {MovementStrafeRight, SaveFacingEast, 0, 1},  {MovementStrafeRight, SaveFacingWest, 0, -1},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const MoveCase *c = &cases[i];
        MovementResult r = movementApply(c->action, c->facing);
        char label[64];
        snprintf(label, sizeof(label), "case %zu (action=%d facing=%s)", i, c->action, facingName(c->facing));
        check(label, r.deltaCol == c->deltaCol && r.deltaRow == c->deltaRow && r.facing == c->facing);
    }
}

static void testBounds(void) {
    checkU32("yendor2 colMin", movementBounds(GameYendor2)->colMin, 0x28);
    checkU32("yendor2 colMax", movementBounds(GameYendor2)->colMax, 0x2F7);
    checkU32("yendor2 rowMin", movementBounds(GameYendor2)->rowMin, 0x18);
    checkU32("yendor2 rowMax", movementBounds(GameYendor2)->rowMax, 0x77);
    checkU32("yendor3 rowMax is taller (168-row map)", movementBounds(GameYendor3)->rowMax, 0x8F);
    checkU32("yendor2 and yendor3 share the same column range", movementBounds(GameYendor3)->colMax,
             movementBounds(GameYendor2)->colMax);

    check("yendor2: 40,24 (top-left corner) is in bounds", movementInBounds(GameYendor2, 0x28, 0x18));
    check("yendor2: 39,24 is out of bounds (one col short)", !movementInBounds(GameYendor2, 0x27, 0x18));
    check("yendor2: 40,120 is out of bounds (one row past)", !movementInBounds(GameYendor2, 0x28, 0x78));
    check("yendor3: 40,143 is in bounds (taller map)", movementInBounds(GameYendor3, 0x28, 0x8F));
    check("yendor3: 40,144 is out of bounds", !movementInBounds(GameYendor3, 0x28, 0x90));
}

static void testFloorClassification(void) {
    /* Chapter 2: void {0,1}, blocked [2,15], normal [16,57], blocked (58+, unbounded). */
    check("yendor2 wall 0 is void", movementClassifyFloorType(GameYendor2, 0) == MovementFloorVoid);
    check("yendor2 wall 1 is void", movementClassifyFloorType(GameYendor2, 1) == MovementFloorVoid);
    check("yendor2 wall 2 is blocked", movementClassifyFloorType(GameYendor2, 2) == MovementFloorBlocked);
    check("yendor2 wall 15 is blocked (end of first blocked band)",
          movementClassifyFloorType(GameYendor2, 15) == MovementFloorBlocked);
    check("yendor2 wall 16 is normal (start of normal band)",
          movementClassifyFloorType(GameYendor2, 16) == MovementFloorNormal);
    check("yendor2 wall 57 is normal (end of normal band)",
          movementClassifyFloorType(GameYendor2, 57) == MovementFloorNormal);
    check("yendor2 wall 58 is blocked again (past the real wall-type table)",
          movementClassifyFloorType(GameYendor2, 58) == MovementFloorBlocked);
    check("yendor2 wall 1000 is still blocked (unbounded past-table band)",
          movementClassifyFloorType(GameYendor2, 1000) == MovementFloorBlocked);

    /* Chapter 3: void {0,1}, blocked [2,99], normal [100,199], blocked [200,299], normal (300+, unbounded). */
    check("yendor3 wall 0 is void", movementClassifyFloorType(GameYendor3, 0) == MovementFloorVoid);
    check("yendor3 wall 1 is void", movementClassifyFloorType(GameYendor3, 1) == MovementFloorVoid);
    check("yendor3 wall 2 is blocked", movementClassifyFloorType(GameYendor3, 2) == MovementFloorBlocked);
    check("yendor3 wall 99 is blocked (end of first blocked band)",
          movementClassifyFloorType(GameYendor3, 99) == MovementFloorBlocked);
    check("yendor3 wall 100 is normal", movementClassifyFloorType(GameYendor3, 100) == MovementFloorNormal);
    check("yendor3 wall 199 is normal", movementClassifyFloorType(GameYendor3, 199) == MovementFloorNormal);
    check("yendor3 wall 200 is blocked again", movementClassifyFloorType(GameYendor3, 200) == MovementFloorBlocked);
    check("yendor3 wall 299 is blocked", movementClassifyFloorType(GameYendor3, 299) == MovementFloorBlocked);
    check("yendor3 wall 300 is normal (unbounded past-table band)",
          movementClassifyFloorType(GameYendor3, 300) == MovementFloorNormal);

    /* Chapter 2's impassable floor types are three disjoint pieces, not one range. */
    check("yendor2 floor type 20 is passable", !movementIsFloorTypeImpassable(GameYendor2, 20));
    check("yendor2 floor type 21 is impassable", movementIsFloorTypeImpassable(GameYendor2, 21));
    check("yendor2 floor type 35 is impassable", movementIsFloorTypeImpassable(GameYendor2, 35));
    check("yendor2 floor type 36 is passable (gap in the range)", !movementIsFloorTypeImpassable(GameYendor2, 36));
    check("yendor2 floor type 37 is impassable (lone value)", movementIsFloorTypeImpassable(GameYendor2, 37));
    check("yendor2 floor type 38 is passable (gap in the range)", !movementIsFloorTypeImpassable(GameYendor2, 38));
    check("yendor2 floor type 39 is impassable", movementIsFloorTypeImpassable(GameYendor2, 39));
    check("yendor2 floor type 42 is impassable", movementIsFloorTypeImpassable(GameYendor2, 42));
    check("yendor2 floor type 43 is passable", !movementIsFloorTypeImpassable(GameYendor2, 43));

    check("yendor3 floor type 199 is passable", !movementIsFloorTypeImpassable(GameYendor3, 199));
    check("yendor3 floor type 200 is impassable", movementIsFloorTypeImpassable(GameYendor3, 200));
    check("yendor3 floor type 399 is impassable", movementIsFloorTypeImpassable(GameYendor3, 399));
    check("yendor3 floor type 400 is passable", !movementIsFloorTypeImpassable(GameYendor3, 400));
}

static void testSpecialWallType(void) {
    check("yendor2: 5 is not special", !movementIsSpecialWallType(GameYendor2, 5));
    check("yendor2: 6 is special", movementIsSpecialWallType(GameYendor2, 6));
    check("yendor2: 11 is special", movementIsSpecialWallType(GameYendor2, 11));
    check("yendor2: 12 is not special", !movementIsSpecialWallType(GameYendor2, 12));

    check("yendor3: 199 is not special", !movementIsSpecialWallType(GameYendor3, 199));
    check("yendor3: 200 is special", movementIsSpecialWallType(GameYendor3, 200));
    check("yendor3: 299 is special", movementIsSpecialWallType(GameYendor3, 299));
    check("yendor3: 300 is not special", !movementIsSpecialWallType(GameYendor3, 300));
    /* The structural parallel: Chapter 3's whole special range sits inside its own "blocked" range. */
    check("yendor3's special range is entirely 'blocked' floor type too",
          movementClassifyFloorType(GameYendor3, 200) == MovementFloorBlocked &&
              movementClassifyFloorType(GameYendor3, 299) == MovementFloorBlocked);
}

static void testClassifyCell(void) {
    /* wallType=30 is in Chapter 2's normal band [16,57]; floorType=10 is passable. */
    check("a door always blocks regardless of tile types",
          movementClassifyCell(GameYendor2, 30, 10, true, false) == MovementCellDoor);
    check("a door blocks even with forceMove set",
          movementClassifyCell(GameYendor2, 30, 10, true, true) == MovementCellDoor);

    check("special wall type always enters, door check takes priority though",
          movementClassifyCell(GameYendor2, 6, 10, false, false) == MovementCellSpecial);

    check("void wall type blocks silently", movementClassifyCell(GameYendor2, 0, 0, false, false) == MovementCellVoid);
    /* wallType=13 is in the blocked band [2,15] but outside the special sub-range [6,11]. */
    check("blocked wall type (with bump)", movementClassifyCell(GameYendor2, 13, 10, false, false) == MovementCellBlocked);
    check("past-table wall type is also blocked (unbounded high band)",
          movementClassifyCell(GameYendor2, 1000, 10, false, false) == MovementCellBlocked);
    check("normal wall type but impassable floor type blocks",
          movementClassifyCell(GameYendor2, 30, 25, false, false) == MovementCellBlocked);
    check("normal wall type and passable floor type is clear",
          movementClassifyCell(GameYendor2, 30, 10, false, false) == MovementCellClear);

    check("forceMove bypasses ordinary blocked wall type",
          movementClassifyCell(GameYendor2, 13, 10, false, true) == MovementCellClear);
    check("forceMove bypasses impassable floor type",
          movementClassifyCell(GameYendor2, 30, 25, false, true) == MovementCellClear);
    check("forceMove does not bypass the door check",
          movementClassifyCell(GameYendor2, 30, 10, true, true) == MovementCellDoor);
    check("forceMove does not bypass the special-cell check",
          movementClassifyCell(GameYendor2, 6, 10, false, true) == MovementCellSpecial);

    check("yendor3: a page-2 wall type (special) takes priority over its own 'blocked' floor classification",
          movementClassifyCell(GameYendor3, 250, 300, false, false) == MovementCellSpecial);
}

static WorldMap g_map;

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return worldMapReadWorldDatFile(&g_map, game, path);
}

static void testRealYendor2(void) {
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game")) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    /* CURGAME's real saved position: world X=166, Y=36. */
    check("the party's real saved position is within the playable bounds", movementInBounds(GameYendor2, 166, 36));

    uint16_t a = worldMapTileA(&g_map, 36, 166);
    uint16_t b = worldMapTileB(&g_map, 36, 166);
    MovementCellOutcome outcome = movementClassifyCell(GameYendor2, a, b, false, false);
    check("the party's real saved position is not a silently-void cell", outcome != MovementCellVoid);
}

static void testRealYendor3(void) {
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game")) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    checkU32("yendor3 map row count still matches the taller bounds", g_map.rowCount, WorldMapRowsYendor3);
    check("yendor3's playable row bound is inside the parsed map", movementBounds(GameYendor3)->rowMax < g_map.rowCount);
}

int main(void) {
    testTurning();
    testMoveDeltas();
    testBounds();
    testFloorClassification();
    testSpecialWallType();
    testClassifyCell();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

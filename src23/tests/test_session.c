/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_session test_session.c ../session.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
 *       ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c \
 *       ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../spellrecord.c && ./test_session
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "session.h"

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

static void place(GameSession *s, int x, int y, uint16_t facing) {
    s->x = x;
    s->y = y;
    s->facing = facing;
    sessionRebuild(s);
}

static void testGame(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    GameSession *s = sessionNew(game, world, size);
    check(label, s != NULL);
    if (!s) {
        return;
    }
    check("the party is the four heroes and the window is built", sessionPartyRecord(s, 0) && sessionPartyRecord(s, 3) && !sessionPartyRecord(s, 4) &&
                                                                   dungeonGridCellAtWorldPos(&s->grid, s->x, s->y) != NULL);

    uint16_t facing = s->facing;
    check("turning left changes the facing and nothing else", sessionMove(s, MovementTurnLeft) == SessionStepTurned && s->facing != facing);
    sessionMove(s, MovementTurnRight);
    check("... and right turns back", s->facing == facing);

    int startX = s->x, startY = s->y;
    unsigned moved = 0;
    for (int i = 0; i < 20; i++) {
        moved += sessionMove(s, MovementForward) == SessionStepMoved;
    }
    check("walking forward moves the party until a wall stops it", moved >= 1 && (s->x != startX || s->y != startY) && moved < 20);

    RestOutcome rest = sessionRest(s);
    check("resting uses up the time (eight hours or an interruption)", !rest.refused && (rest.interrupted || rest.minutes == 480));

    /* random play on every game: the invariants must hold */
    unsigned seed = 1, bad = 0;
    GameSession *soak = sessionNew(game, world, size);
    for (unsigned i = 0; i < 1500 && soak; i++) {
        seed = seed * 1103515245u + 12345u;
        unsigned pick = (seed >> 16) % 14;
        if (pick < 6) {
            sessionMove(soak, MovementForward);
        } else if (pick < 8) {
            sessionMove(soak, MovementTurnLeft);
        } else if (pick < 9) {
            sessionMove(soak, MovementTurnRight);
        } else if (pick < 10) {
            sessionMove(soak, MovementBackward);
        } else if (pick < 13) {
            sessionAttack(soak);
        } else {
            sessionRest(soak);
        }
        ViewScene scene;
        DungeonGridCell cells[ViewportCellCount];
        sessionScene(soak, &scene, cells);
        bad += !movementInBounds(game, (uint16_t)soak->x, (uint16_t)soak->y);
        bad += soak->combat.active && soak->combat.wiped;
        bad += soak->clock.minutes >= 1440;
    }
    check("1500 random commands keep the party in bounds and the clock in a day", bad == 0);
    sessionFree(soak);
    sessionFree(s);
    free(world);
}

static void testCombat(const char *envName, const char *defaultDir) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        return;
    }
    GameSession *s = sessionNew(GameYendor2, world, size);
    place(s, 161, 31, SaveFacingNorth); /* spiders walk the corridors around here */
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    unsigned turns = 0;
    while (!s->combat.active && turns < 300) { /* turn on the spot and let them come */
        sessionScene(s, &scene, cells);
        sessionMove(s, turns & 1 ? MovementTurnRight : MovementTurnLeft);
        turns++;
    }
    check("monsters reach a standing party and start a combat", s->combat.active);
    unsigned swings = 0;
    while (s->combat.active && swings < 400) {
        sessionAttack(s);
        swings++;
    }
    check("the combat ends one way or the other", !s->combat.active);
    if (!s->combat.wiped) {
        check("a won combat leaves the monster out of the pool and the move command free again", sessionMove(s, MovementTurnLeft) == SessionStepTurned);
    } else {
        check("a wiped party cannot move", sessionMove(s, MovementTurnLeft) == SessionStepBusy);
    }
    sessionFree(s);

    /* loot and unlock: the chest ahead of (163, 43) facing north has contents; the second visit finds it empty */
    s = sessionNew(GameYendor2, world, size);
    place(s, 163, 43, SaveFacingNorth);
    unsigned first = sessionLoot(s), second = sessionLoot(s);
    check("a chest is looted once", first > 0 && second == 0);
    place(s, 163, 43, SaveFacingNorth);
    check("the taken slots stay taken after the window is rebuilt", sessionLoot(s) == 0);
    InteractUnlockOutcome nothing = sessionUnlock(s, 0xFFC0, 0);
    check("unlocking with nothing ahead of interest reports a result without crashing", nothing.result <= InteractUnlockLocked);
    sessionFree(s);
    free(world);
}

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: a session starts");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: a session starts");
    testCombat("YENDOR2_GAME_DIR", "../../yendor2/game");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

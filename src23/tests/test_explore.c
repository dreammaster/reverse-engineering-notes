/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_explore test_explore.c ../explore.c ../savegame.c && ./test_explore
 */
#include <stdio.h>
#include <string.h>

#include "explore.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static unsigned countExplored(SaveGame *save, GameKind game) {
    unsigned n = 0;
    unsigned rows = game == GameYendor3 ? 168 : 144;
    for (unsigned y = 0; y < rows; y++) {
        for (unsigned x = 0; x < 800; x++) {
            n += exploreIsExplored(save, (int)x, (int)y);
        }
    }
    return n;
}

static void testBitmap(void) {
    static SaveGame save;
    saveGameInit(&save, GameYendor2);
    check("fresh saves are unexplored", !exploreIsExplored(&save, 5, 5));
    check("marking a cell reports it new", exploreMarkCell(&save, 5, 5));
    check("...once", !exploreMarkCell(&save, 5, 5) && exploreIsExplored(&save, 5, 5));
    uint8_t *row = saveGameRecord(&save, SaveSectionExploredMap, 5);
    check("bit 7 - x%8 of byte x/8: column 5 is 0x04 of byte 0", row[0] == 0x04);
    exploreMarkCell(&save, 8, 5);
    check("column 8 is the top bit of byte 1", row[1] == 0x80);
    check("neighbours stay clear", !exploreIsExplored(&save, 4, 5) && !exploreIsExplored(&save, 6, 5));
    check("outside the bitmap is never explored or markable", !exploreMarkCell(&save, -1, 0) && !exploreMarkCell(&save, 800, 0) &&
                                                                   !exploreMarkCell(&save, 0, 144) && !exploreIsExplored(&save, 0, 500));
}

static void testReveal(void) {
    static SaveGame save;
    ExploreReveal r;
    saveGameInit(&save, GameYendor2);
    exploreRevealAroundPlayer(&save, 100, 50, SaveFacingNorth, &r);
    check("facing north reveals the row ahead and the party's row, three wide", r.count == 6 && countExplored(&save, GameYendor2) == 6);
    check("...order -1, +1, 0 within a row", r.cells[0].x == 99 && r.cells[0].y == 49 && r.cells[1].x == 101 && r.cells[2].x == 100 &&
                                                  r.cells[3].y == 50);
    exploreRevealAroundPlayer(&save, 100, 50, SaveFacingNorth, &r);
    check("revealing again finds nothing new", r.count == 0);

    saveGameInit(&save, GameYendor2);
    exploreRevealAroundPlayer(&save, 100, 50, SaveFacingSouth, &r);
    check("south: rows y+1 and y", exploreIsExplored(&save, 100, 51) && exploreIsExplored(&save, 99, 50) && !exploreIsExplored(&save, 100, 49));
    saveGameInit(&save, GameYendor2);
    exploreRevealAroundPlayer(&save, 100, 50, SaveFacingEast, &r);
    check("east: columns x+1 and x", exploreIsExplored(&save, 101, 49) && exploreIsExplored(&save, 100, 51) && !exploreIsExplored(&save, 99, 50));
    saveGameInit(&save, GameYendor2);
    exploreRevealAroundPlayer(&save, 100, 50, SaveFacingWest, &r);
    check("west: columns x-1 and x", exploreIsExplored(&save, 99, 49) && exploreIsExplored(&save, 100, 51) && !exploreIsExplored(&save, 101, 50));
    saveGameInit(&save, GameYendor2);
    exploreRevealAroundPlayer(&save, 0, 0, SaveFacingNorth, &r);
    check("cells off the map edge are skipped", r.count == 2);
}

int main(void) {
    testBitmap();
    testReveal();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

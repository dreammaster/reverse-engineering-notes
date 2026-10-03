/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_uiregions test_uiregions.c ../uiregions.c && ./test_uiregions
 */
#include <stdio.h>

#include "uiregions.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

int main(void) {
    check("the viewport is region 1 of the main screen", uiRegionHit(GameYendor2, UiRegionsDungeonMain, 100, 50) == 1);
    check("...its corners are inclusive", uiRegionHit(GameYendor2, UiRegionsDungeonMain, 8, 8) == 1 && uiRegionHit(GameYendor2, UiRegionsDungeonMain, 231, 143) == 1);
    check("...one pixel outside misses it", uiRegionHit(GameYendor2, UiRegionsDungeonMain, 7, 8) == 0 && uiRegionHit(GameYendor2, UiRegionsDungeonMain, 232, 8) == 0);
    check("the portrait strip is region 6", uiRegionHit(GameYendor2, UiRegionsDungeonMain, 100, 170) == 6);
    check("the four icon buttons", uiRegionHit(GameYendor2, UiRegionsDungeonIconRow, 245, 70) == 1 && uiRegionHit(GameYendor2, UiRegionsDungeonIconRow, 300, 70) == 4);
    check("party panel 2's portrait", uiRegionHit(GameYendor2, UiRegionsPartyPanels, 70, 160) == 11);
    check("the layouts of both games agree on the main screen", uiRegionHit(GameYendor3, UiRegionsDungeonMain, 100, 50) == 1 &&
                                                                     uiRegionHit(GameYendor3, UiRegionsDungeonMain, 100, 170) == 6);
    unsigned c2, c3;
    uiRegionEntries(GameYendor2, UiRegionsInventoryGrid, &c2);
    uiRegionEntries(GameYendor3, UiRegionsInventoryGrid, &c3);
    check("Chapter 3's inventory grid has one slot fewer", c2 == 21 && c3 == 20);
    check("an unknown table has no entries", uiRegionEntries(GameYendor2, UiRegionTableCount, &c2) == 0 && c2 == 0);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

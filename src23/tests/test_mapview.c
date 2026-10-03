/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_mapview test_mapview.c ../mapview.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_mapview
 */
#include <stdio.h>
#include <string.h>

#include "mapview.h"

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void setMember(SaveGame *save, unsigned slot, uint16_t id, uint16_t mapping, uint16_t navigation, uint16_t survival,
                      uint16_t status) {
    saveHeaderSetU16(save, SaveHeaderPartySlots + slot * 2, id);
    if (id == 0) {
        return;
    }
    uint8_t *r = saveGamePartyRecordById(save, id);
    partySetStat(r, PartyStatMapping, mapping);
    partySetStat(r, PartyStatNavigation, navigation);
    partySetStat(r, PartyStatSurvival, survival);
    partySetU16(r, PartyFieldStatusFlags, status);
}

static void testFieldIdentities(void) {
    /* The averaged record offsets are exactly the Mapping, Navigation and Survival stats. */
    checkU32("+0x64 is Mapping", PartyFieldStats + PartyStatMapping * 2, 0x64);
    checkU32("+0x66 is Navigation", PartyFieldStats + PartyStatNavigation * 2, 0x66);
    checkU32("+0x58 is Survival", PartyFieldStats + PartyStatSurvival * 2, 0x58);
}

static void testAverages(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    setMember(&save, 0, 1, 70, 20, 10, 0);
    setMember(&save, 1, 2, 51, 31, 21, PartyStatusPoisoned);
    setMember(&save, 2, 3, 99, 99, 99, PartyStatusDead);
    setMember(&save, 3, 4, 40, 10, 8, 0x0005);
    PartyStatAverages a = partyAverageStatTiers(&save);
    checkU32("the dead member is skipped: three counted", a.counted, 3);
    checkU32("mapping (70 + 51 + 40) / 3", a.mapping, 53);
    checkU32("navigation (20 + 31 + 10) / 3", a.navigation, 20);
    checkU32("survival (10 + 21 + 8) / 3", a.survival, 13);

    setMember(&save, 1, 0, 0, 0, 0, 0);
    a = partyAverageStatTiers(&save);
    checkU32("an empty slot stops the scan: only the first member", a.counted, 1);
    checkU32("...so the average is his own", a.mapping, 70);

    setMember(&save, 0, 0, 0, 0, 0, 0);
    a = partyAverageStatTiers(&save);
    checkU32("an empty party averages to 0", a.mapping + a.navigation + a.survival + a.counted, 0);
}

static void testTiers(void) {
    bool redraw;
    checkU32("below 45 sets nothing", mapviewApplyTiers(0, 44, &redraw), 0);
    checkU32("45", mapviewApplyTiers(0, 45, &redraw), MapFlagExtended);
    checkU32("50", mapviewApplyTiers(0, 50, &redraw), MapFlagExtended | MapFlagLargeCapable);
    checkU32("60", mapviewApplyTiers(0, 60, &redraw), MapFlagExtended | MapFlagLargeCapable | MapFlagLocalMap);
    checkU32("70", mapviewApplyTiers(0, 70, &redraw),
             MapFlagExtended | MapFlagLargeCapable | MapFlagLocalMap | MapFlagCoordinates);
    checkU32("80 and above", mapviewApplyTiers(0, 90, &redraw),
             MapFlagExtended | MapFlagLargeCapable | MapFlagLocalMap | MapFlagCoordinates | MapFlagFullOverview);

    checkU32("the display mode bits survive a recompute", mapviewApplyTiers(MapFlagSmall, 60, &redraw) & 0x7000, MapFlagSmall);
    check("...and old tier bits are dropped", (mapviewApplyTiers(MapFlagLocalMap | MapFlagSmall, 10, &redraw) & MapFlagLocalMap) == 0);

    uint16_t flags = mapviewApplyTiers(MapFlagLarge | MapFlagLargeCapable, 40, &redraw);
    check("a large minimap that lost its capability gains the hidden bit (the large bit is NOT cleared -- reproduced)",
          flags == (MapFlagLarge | MapFlagHidden) && redraw);
    flags = mapviewApplyTiers(MapFlagLarge | MapFlagLargeCapable, 55, &redraw);
    check("one that kept it stays large", (flags & MapFlagLarge) && !redraw);
}

static void testGates(void) {
    check("a skilled party opens the local map", mapviewLocalMapAllowed(GameYendor2, MapFlagLocalMap, 0));
    check("an unskilled one does not", !mapviewLocalMapAllowed(GameYendor2, MapFlagExtended, 0));
    check("the map editor's call always does", mapviewLocalMapAllowed(GameYendor2, 0, 1));
    check("flag 0x8000 bypasses only in Chapter 3", mapviewLocalMapAllowed(GameYendor3, 0, 0x8000) &&
                                                         !mapviewLocalMapAllowed(GameYendor2, 0, 0x8000));
    check("the overview needs its own tier", mapviewOverviewAllowed(MapFlagFullOverview) &&
                                                  !mapviewOverviewAllowed(MapFlagLocalMap | MapFlagCoordinates));
}

static void testPage(void) {
    MapviewPage p = mapviewPage(0, 0, SaveFacingNorth);
    check("the origin cell is page 0", p.pageIndex == 0 && p.originX == 0 && p.originY == 0);
    check("...with the marker under the title line", p.markerPixelX == 0 && p.markerPixelY == 8 && p.arrowPicture == 0);

    p = mapviewPage(659, 46, SaveFacingEast); /* Chapter 2's destination 1 */
    checkU32("page index (46 / 24) * 20 + 659 / 40", p.pageIndex, 1 * 20 + 16);
    check("origin 640, 24", p.originX == 640 && p.originY == 24);
    check("marker at (19 * 8, 23 * 8)", p.markerPixelX == 19 * 8 && p.markerPixelY == 23 * 8);
    checkU32("east arrow", p.arrowPicture, 1);
    checkU32("south arrow", mapviewPage(5, 5, SaveFacingSouth).arrowPicture, 2);
    checkU32("west arrow is the default", mapviewPage(5, 5, SaveFacingWest).arrowPicture, 3);
    checkU32("the last page of a 144-row world is 119", mapviewPage(799, 143, SaveFacingNorth).pageIndex, 119);
    checkU32("and of a 168-row one 139", mapviewPage(799, 167, SaveFacingNorth).pageIndex, 139);
}

static void testOverviewMarker(void) {
    int x, y;
    check("outside the overview", !mapviewOverviewMarker(159, 100, &x, &y) && !mapviewOverviewMarker(100, 47, &x, &y) &&
                                      !mapviewOverviewMarker(640, 100, &x, &y) && !mapviewOverviewMarker(200, 240, &x, &y));
    check("its corner", mapviewOverviewMarker(160, 48, &x, &y) && x == 28 && y == 23);
    check("one cell short of the next block stays in the block", mapviewOverviewMarker(199, 71, &x, &y) && x == 28 && y == 23);
    check("the next block", mapviewOverviewMarker(200, 72, &x, &y) && x == 52 && y == 43);
    check("the far corner", mapviewOverviewMarker(639, 239, &x, &y) && x == 11 * 24 + 28 && y == 7 * 20 + 23);
}

int main(void) {
    testFieldIdentities();
    testAverages();
    testTiers();
    testGates();
    testPage();
    testOverviewMarker();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

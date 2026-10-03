/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_mount test_mount.c ../mount.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c ../interact.c ../lockcatalog.c ../worldobjects.c ../movement.c ../worldmap.c ../worldmap_stdio.c && ./test_mount
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mount.h"
#include "party.h"

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

static void testCheck(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    unsigned slot = 9;
    check("commands outside 2-5 do nothing", mountCheck(r, 1, 600, &slot) == MountCheckNotAMount && mountCheck(r, 6, 600, &slot) == MountCheckNotAMount);
    check("an unlearned mount", mountCheck(r, 2, 600, &slot) == MountCheckNotLearned);
    partySetU16(r, PartyFieldAbilities, 0x4000 | 0x1000); /* GIANT EAGLE and MAGIC DRAGON */
    check("the first command is the first LEARNED mount (the eagle, slot 1)", mountCheck(r, 2, 600, &slot) == MountCheckOk && slot == 1);
    check("the second is the dragon, but it flies only at night", mountCheck(r, 3, 600, &slot) == MountCheckWrongTime && slot == 3);
    check("...at night it can", mountCheck(r, 3, 100, &slot) == MountCheckOk && mountCheck(r, 3, 1141, &slot) == MountCheckOk);
    check("07:00 and 19:00 count as day", mountCheck(r, 3, 420, &slot) == MountCheckWrongTime && mountCheck(r, 3, 1140, &slot) == MountCheckWrongTime &&
                                               mountCheck(r, 3, 419, &slot) == MountCheckOk);
    check("a third mount is not learned", mountCheck(r, 4, 600, &slot) == MountCheckNotLearned);
    mountSpendCharge(r, 1);
    check("the eagle has two daily charges", mountCheck(r, 2, 600, &slot) == MountCheckOk);
    mountSpendCharge(r, 1);
    check("...then none", mountCheck(r, 2, 600, &slot) == MountCheckNoCharges);

    partySetU16(r, PartyFieldAbilities, 0x2000);
    check("the flying rug flies by day only", mountCheck(r, 2, 600, &slot) == MountCheckOk && mountCheck(r, 2, 100, &slot) == MountCheckWrongTime);
    partySetU16(r, PartyFieldAbilities, 0x8000);
    check("the pegasus flies at any time", mountCheck(r, 2, 600, &slot) == MountCheckOk && mountCheck(r, 2, 100, &slot) == MountCheckOk);
    mountSpendCharge(r, 0);
    check("...once a day", mountCheck(r, 2, 600, &slot) == MountCheckNoCharges);
}

static void testBox(void) {
    MountBox b = mountRevealBox(64);
    check("Navigation < 65: 11 x 7", b.columns == 11 && b.rows == 7 && b.pixelX == 76 && b.pixelY == 48);
    b = mountRevealBox(65);
    check("65: 17 x 9", b.columns == 17 && b.rows == 9);
    b = mountRevealBox(80);
    check("80: 23 x 13", b.columns == 23 && b.rows == 13);
    b = mountRevealBox(95);
    check("95: 27 x 17 at (12, 8)", b.columns == 27 && b.rows == 17 && b.pixelX == 12 && b.pixelY == 8);

    b = mountRevealBox(0);
    int ox, oy, cx, cy;
    mountBoxOrigin(&b, 100, 50, &ox, &oy);
    check("the box is centred on the party", ox == 95 && oy == 47);
    check("the top-left pixel is the origin cell", mountCellFromClick(&b, ox, oy, 76, 48, &cx, &cy) && cx == 95 && cy == 47);
    check("the centre cell is the party's", mountCellFromClick(&b, ox, oy, 76 + 5 * 8 + 3, 48 + 3 * 8 + 3, &cx, &cy) && cx == 100 && cy == 50);
    check("outside the box", !mountCellFromClick(&b, ox, oy, 75, 48, &cx, &cy) && !mountCellFromClick(&b, ox, oy, 76 + 11 * 8, 48, &cx, &cy) &&
                                  !mountCellFromClick(&b, ox, oy, 76, 48 + 7 * 8, &cx, &cy) && !mountCellFromClick(&b, ox, oy, 76, 47, &cx, &cy));
}

static void testVerdict(void) {
    check("an ordinary explored clear cell", mountTravelVerdict(GameYendor2, true, 0, 0, true, false, false, InteractOutcomeNone) == MountTravelOk);
    check("unexplored", mountTravelVerdict(GameYendor2, true, 0, 0, false, false, false, InteractOutcomeNone) == MountTravelUnexplored);
    check("an impassable floor type, then wall type", mountTravelVerdict(GameYendor2, true, 0, 0, true, true, true, InteractOutcomeNone) == MountTravelFloorBlocked &&
                                                          mountTravelVerdict(GameYendor2, true, 0, 0, true, false, true, InteractOutcomeNone) == MountTravelWallBlocked);
    check("a trap or trigger object refuses", mountTravelVerdict(GameYendor2, true, 0, 0, true, false, false, InteractOutcomeCurgameFlag10) == MountTravelInteractive &&
                                                  mountTravelVerdict(GameYendor2, true, 0, 0, true, false, false, InteractOutcomeCurgameFlag8) == MountTravelInteractive &&
                                                  mountTravelVerdict(GameYendor2, true, 0, 0, true, false, false, InteractOutcomeLockMagical) == MountTravelOk);
    check("Chapter 3 refuses other pages when either attribute is 2; Chapter 2 does not",
          mountTravelVerdict(GameYendor3, false, 2, 0, true, false, false, InteractOutcomeNone) == MountTravelOutside &&
              mountTravelVerdict(GameYendor3, false, 0, 2, true, false, false, InteractOutcomeNone) == MountTravelOutside &&
              mountTravelVerdict(GameYendor3, true, 2, 2, true, false, false, InteractOutcomeNone) == MountTravelOk &&
              mountTravelVerdict(GameYendor2, false, 2, 2, true, false, false, InteractOutcomeNone) == MountTravelOk);
    check("visibility: same page always; others only from an attribute-1 page and not into a 2",
          mountCellVisible(true, 2, 2) && !mountCellVisible(false, 0, 0) && mountCellVisible(false, 1, 0) && mountCellVisible(false, 1, 1) &&
              !mountCellVisible(false, 1, 2) && !mountCellVisible(false, 2, 0));
}

static uint8_t *loadFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc((size_t)len);
    if (data && fread(data, 1, (size_t)len, f) != (size_t)len) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return data;
}

static void testReal(int chapter) {
    char path[512];
    const char *dir = getenv(chapter == 2 ? "YENDOR2_GAME_DIR" : "YENDOR3_GAME_DIR");
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : (chapter == 2 ? "../../yendor2/game" : "../../yendor3/game"));
    size_t size;
    uint8_t *image = loadFile(path, &size);
    if (!image) {
        printf("SKIP chapter %d page-attribute checks (no WORLD.DAT)\n", chapter);
        g_skipCount++;
        return;
    }
    GameKind game = chapter == 2 ? GameYendor2 : GameYendor3;
    unsigned pages = chapter == 2 ? 120 : 140, counts[3] = {0, 0, 0}, other = 0;
    for (unsigned p = 0; p < pages; p++) {
        uint8_t a = mountPageAttribute(game, image, size, p);
        if (a <= 2) {
            counts[a]++;
        } else {
            other++;
        }
    }
    check("every real page has attribute 0, 1 or 2", other == 0);
    if (chapter == 2) {
        check("Chapter 2 splits 48 / 25 / 47", counts[0] == 48 && counts[1] == 25 && counts[2] == 47);
    } else {
        check("Chapter 3 pages are mostly attribute 2", counts[2] > counts[1] && counts[0] + counts[1] + counts[2] == pages);
    }
    free(image);
}

int main(void) {
    testCheck();
    testBox();
    testVerdict();
    testReal(2);
    testReal(3);

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_storyevent test_storyevent.c ../storyevent.c ../globalflags.c ../worldobjects.c ../movement.c ../savegame.c && ./test_storyevent
 *
 * Also checks the world objects of yendor2/game and yendor3/game WORLD.DAT when present (skipped otherwise; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globalflags.h"
#include "movement.h"
#include "savegame.h"
#include "storyevent.h"
#include "worldobjects.h"

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

static unsigned u16(const uint8_t *p) {
    return (unsigned)(p[0] | (p[1] << 8));
}

static void testObjects(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *world = malloc(size);
    bool read = world && fread(world, 1, size, f) == size;
    fclose(f);
    static WorldObjectTable table;
    if (!read || !worldObjectTableParseWorldDat(&table, game, world, size)) {
        check("WORLD.DAT reads", false);
        return;
    }
    unsigned found = 0, seen[8] = {0};
    bool inBounds = true;
    for (unsigned column = 0; column < WorldObjectColumns; column++) {
        for (unsigned at = u16(table.data + 2 * column);; at += 6) {
            unsigned y = u16(table.data + at);
            if (y == 0xFFFF) {
                break;
            }
            unsigned flags = u16(table.data + at + 2), value = u16(table.data + at + 4);
            if (flags & WorldObjectFlagStoryEvent) {
                found++;
                seen[value < 8 ? value : 7]++;
            }
        }
    }
    if (game == GameYendor2) {
        check("Chapter 2 has exactly one story-event object, with value 1", found == 1 && seen[1] == 1);
    } else {
        check("Chapter 3 has six story-event objects, values 1-6 once each", found == 6 && seen[1] == 1 && seen[2] == 1 && seen[3] == 1 && seen[4] == 1 && seen[5] == 1 && seen[6] == 1);
        for (unsigned v = 1; v <= StoryEventCountYendor3; v++) {
            const StoryEvent *e = storyEvent(game, v);
            inBounds = inBounds && e && movementInBounds(game, (uint16_t)e->x, (uint16_t)e->y);
        }
        check("every Chapter 3 event's destination is inside the playable bounds", inBounds);
    }
    free(world);
}

int main(void) {
    check("Chapter 2 has no scripted events and Chapter 3's values run 1-6",
          !storyEvent(GameYendor2, 1) && !storyEvent(GameYendor3, 0) && !storyEvent(GameYendor3, 7) && storyEvent(GameYendor3, 1) && storyEvent(GameYendor3, 6));
    const StoryEvent *one = storyEvent(GameYendor3, 1);
    check("event 1 is a one-time teleport to Thaine facing west", one->doneFlag == 5 && one->x == 397 && one->y == 94 && one->facing == SaveFacingWest && one->clearTracks && one->doneTextAlways);
    const StoryEvent *five = storyEvent(GameYendor3, 5), *six = storyEvent(GameYendor3, 6);
    check("events 5 and 6 enter and leave the artifact realm (flag 0x9D)", five->setFlag == 0x9D && five->requireFlag == 0x91 && six->clearFlag == 0x9D && six->removeLow == 0x17F && six->removeHigh == 0x181);
    check("event 4 swaps item 0xC6 for 0xCD", storyEvent(GameYendor3, 4)->swapFrom == 0xC6 && storyEvent(GameYendor3, 4)->swapTo == 0xCD);

    uint8_t flags[SaveHeaderGlobalFlagsSize];
    memset(flags, 0, sizeof(flags));
    check("the ending needs all three quest flags", !storyEndingReady(flags, sizeof(flags)));
    globalFlagSet(flags, sizeof(flags), StoryEndingFlagA);
    globalFlagSet(flags, sizeof(flags), StoryEndingFlagB);
    check("... two are not enough", !storyEndingReady(flags, sizeof(flags)));
    globalFlagSet(flags, sizeof(flags), StoryEndingFlagC);
    check("... three are", storyEndingReady(flags, sizeof(flags)));

    testObjects(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2's story-event objects");
    testObjects(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3's story-event objects");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

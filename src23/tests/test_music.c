/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_music test_music.c ../music.c && ./test_music
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "music.h"

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

static void testPages(void) {
    check("page index = (y / 24) * 20 + x / 40", musicPageForPosition(0, 0) == 0 && musicPageForPosition(39, 23) == 0 && musicPageForPosition(40, 0) == 1 &&
                                                   musicPageForPosition(0, 24) == 20 && musicPageForPosition(659, 46) == 36);
    unsigned last = 999;
    uint8_t world[8] = {0};
    uint16_t track = 77;
    check("a first look always reports a change", musicRegionChanged(&last, 10, 10, GameYendor2, world, sizeof(world), &track) && last == 0 && track == 0);
    check("staying in the page reports nothing", !musicRegionChanged(&last, 30, 20, GameYendor2, world, sizeof(world), &track));
}

static void testChoice(void) {
    check("a forced track wins, even with ambient music off", musicAmbientChoice(9, 3, 4, 600, 0) == 9);
    check("by day the destination's day track", musicAmbientChoice(0, 3, 4, 600, 0x2000) == 3);
    check("by night its night track", musicAmbientChoice(0, 3, 4, 100, 0x2000) == 4 && musicAmbientChoice(0, 3, 4, 1141, 0x2000) == 4);
    check("07:00 and 19:00 are day", musicAmbientChoice(0, 3, 4, 420, 0x2000) == 3 && musicAmbientChoice(0, 3, 4, 1140, 0x2000) == 3 &&
                                          musicAmbientChoice(0, 3, 4, 419, 0x2000) == 4);
    check("nothing when ambient music is not allowed or the track is 0", musicAmbientChoice(0, 3, 4, 600, 0) == 0 && musicAmbientChoice(0, 0, 4, 600, 0x2000) == 0);
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
        printf("SKIP chapter %d music table (no WORLD.DAT)\n", chapter);
        g_skipCount++;
        return;
    }
    GameKind game = chapter == 2 ? GameYendor2 : GameYendor3;
    unsigned pages = chapter == 2 ? 120 : 140, max = 0, nonzero = 0;
    for (unsigned p = 0; p < pages; p++) {
        uint16_t t = musicPageTrack(game, image, size, p);
        nonzero += t != 0;
        if (t > max) {
            max = t;
        }
    }
    check("tracks stay within the real range and many pages have music", max == (chapter == 2 ? 17u : 23u) && nonzero > 40);
    check("Chapter 2 page 24 plays track 11", chapter != 2 || musicPageTrack(game, image, size, 24) == 11);
    free(image);
}

int main(void) {
    testPages();
    testChoice();
    testReal(2);
    testReal(3);

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

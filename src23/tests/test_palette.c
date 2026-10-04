/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_palette test_palette.c ../palette.c && ./test_palette
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palette.h"

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

static void testFade(void) {
    uint8_t block[PaletteBytes], window[DayNightWindowBytes];
    for (unsigned i = 0; i < PaletteBytes; i++) {
        block[i] = (uint8_t)(i % 64);
    }
    DayNightFade fade;
    dayNightFadeBegin(&fade, 360);
    bool more = dayNightFadeStep(&fade, block, window);
    check("dawn: the first window is the first 32 colours", more && memcmp(window, block, 96) == 0);
    dayNightFadeStep(&fade, block, window);
    check("...then one colour (3 bytes) further each step", memcmp(window, block + 3, 96) == 0);
    unsigned steps = 2;
    while (dayNightFadeStep(&fade, block, window)) {
        steps++;
    }
    check("113 steps in all", steps + 1 == 113);
    check("...the last dawn window starts at colour 112", memcmp(window, block + 336, 96) == 0);

    dayNightFadeBegin(&fade, 1080);
    dayNightFadeStep(&fade, block, window);
    check("dusk starts at colour 111 (byte 333) and moves down", memcmp(window, block + 0x14D, 96) == 0);
    dayNightFadeStep(&fade, block, window);
    check("...one colour further down", memcmp(window, block + 0x14D - 3, 96) == 0);
    unsigned count = 2;
    while (dayNightFadeStep(&fade, block, window)) {
        count++;
    }
    check("113 steps; the last reads one colour before the table (zeros), then the table",
          count + 1 == 113 && window[0] == 0 && window[2] == 0 && memcmp(window + 3, block, 93) == 0);
}

static void testCycle(void) {
    uint8_t block[PaletteBytes], frame[CycleBytes];
    for (unsigned i = 0; i < PaletteBytes; i++) {
        block[i] = (uint8_t)(i * 7 % 64);
    }
    const uint8_t *t = block + 3 * CycleFirstColour;
    paletteCycleFrame(block, 0, frame);
    check("phase 0 is the table as stored", memcmp(frame, t, CycleBytes) == 0);
    paletteCycleFrame(block, 1, frame);
    check("phase 1 rotates each group of four colours left by one",
          memcmp(frame, t + 3, 9) == 0 && memcmp(frame + 9, t, 3) == 0 && memcmp(frame + 12, t + 15, 9) == 0 && memcmp(frame + 21, t + 12, 3) == 0);
    paletteCycleFrame(block, 2, frame);
    check("phase 2 by two", memcmp(frame, t + 6, 6) == 0 && memcmp(frame + 6, t, 6) == 0 && memcmp(frame + 36, t + 42, 6) == 0);
    paletteCycleFrame(block, 3, frame);
    check("phase 3 by three", memcmp(frame, t + 9, 3) == 0 && memcmp(frame + 3, t, 9) == 0);
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    bool valid = true;
    uint8_t block[PaletteBytes];
    for (unsigned b = 0; b < 4; b++) {
        bool ok = fseek(f, (long)paletteBlockOffset(game, b), SEEK_SET) == 0 && fread(block, 1, sizeof(block), f) == sizeof(block);
        for (unsigned i = 0; ok && i < sizeof(block); i++) {
            ok = block[i] <= 63;
        }
        valid = valid && ok;
    }
    fclose(f);
    check(label, valid);
}

int main(void) {
    testFade();
    testCycle();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: the first four palette blocks hold 6-bit values");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: the first four palette blocks hold 6-bit values");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

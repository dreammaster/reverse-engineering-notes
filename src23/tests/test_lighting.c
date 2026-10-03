/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_lighting test_lighting.c ../lighting.c && ./test_lighting
 */
#include <stdio.h>
#include <string.h>

#include "lighting.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static bool same(const int16_t *a, const int16_t *b) {
    return memcmp(a, b, LightingGradientSize * sizeof(int16_t)) == 0;
}

static void testDayCycle(void) {
    int16_t g[LightingGradientSize];
    bool reset;
    LightingInput in = {0, 0, 0, 0x8000};
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("midnight is the darkest: -10..-4", same(g, (int16_t[]){-10, -9, -8, -7, -6, -5, -4}) && !reset);
    in.clockMinutes = 381;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("the first entry lasts through minute 381", g[0] == -10);
    in.clockMinutes = 382;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("...and the second starts at 382", g[0] == -10);
    in.clockMinutes = 392;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("dawn brightens: 392 -> -9..-3", same(g, (int16_t[]){-9, -8, -7, -6, -5, -4, -3}));
    in.clockMinutes = 600;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("Chapter 2 midday has a +1 in the nearest band", same(g, (int16_t[]){1, 0, 0, 0, 0, 0, 0}));
    lightingComputeGradient(GameYendor3, &in, 0, g, &reset);
    check("Chapter 3 midday stops at 0", same(g, (int16_t[]){0, 0, 0, 0, 0, 0, 0}));
    in.clockMinutes = 1443;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("the last entry reaches minute 1443", g[0] == -10 && !reset);
    in.clockMinutes = 1444;
    lightingComputeGradient(GameYendor2, &in, 0, g, &reset);
    check("past the table the clock is reset and the first entry used", reset && g[0] == -10);
}

static void testFlags(void) {
    int16_t g[LightingGradientSize];
    LightingInput in = {4, 4, 600, 0x8000};
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    in.flagsA = 4;
    in.flagsB = 0;
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("flag 4 -> -12..-6", same(g, (int16_t[]){-12, -11, -10, -9, -8, -7, -6}));
    in.flagsA = 2;
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("flag 2 -> -8..-2", same(g, (int16_t[]){-8, -7, -6, -5, -4, -3, -2}));
    in.flagsA = 1;
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("flag 1 -> -7..-1", same(g, (int16_t[]){-7, -6, -5, -4, -3, -2, -1}));
    in.flagsA = 0x8000;
    in.flagsB = 0;
    lightingComputeGradient(GameYendor3, &in, 0, g, NULL);
    check("Chapter 3: 0x8000 -> -12..-6, 0x2000 -> -7..-1", g[0] == -12);
    in.flagsA = 0x2000;
    lightingComputeGradient(GameYendor3, &in, 0, g, NULL);
    check("...", g[0] == -7);
    in.flagsA = 0x4000;
    in.clockMinutes = 0;
    lightingComputeGradient(GameYendor3, &in, 0, g, NULL);
    check("Chapter 3 flag 0x4000 forces noon even at midnight", same(g, (int16_t[]){0, 0, 0, 0, 0, 0, 0}));
}

static void testLightSources(void) {
    int16_t g[LightingGradientSize];
    LightingInput in = {1, 0x200, 0, 0x8000}; /* flag 1: -7..-1; tier k0 */
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("k0 adds 10, 10, 9, 9, 8, 7, 6 to -7..-1: the sum is capped at 0", same(g, (int16_t[]){0, 0, 0, 0, 0, 0, 0}));
    in.flagsA = 4; /* -12..-6 */
    in.flagsB = 0x4000; /* k5: +7 6 5 4 3 2 ... */
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("the dimmest light (k5, +2 everywhere) only lifts the dark a little", same(g, (int16_t[]){-10, -9, -8, -7, -6, -5, -4}));
    in.flagsB = 0x200 | 0x4000;
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("the strongest applicable tier wins (k0 before k5)", g[0] == -2 && g[2] == -1);
    in.flagsB = 0;
    lightingComputeGradient(GameYendor2, &in, 3, g, NULL);
    check("a wall torch in the front row (tier 3) acts as k3 (+4)", g[0] == -12 + 4);
    lightingComputeGradient(GameYendor2, &in, 1, g, NULL);
    check("tier 1 as k5 (+2)", g[0] == -12 + 2);
    in.flagsA = 0;
    in.clockMinutes = 600;
    in.flagsB = 0x200;
    lightingComputeGradient(GameYendor2, &in, 0, g, NULL);
    check("positive deltas are left alone", g[0] == 1);
}

static void testTorchTier(void) {
    uint16_t cells[LightingNearCells] = {0};
    check("no torch", lightingWallTorchTier(cells, 0x8000) == 0);
    cells[4] = 47;
    check("a type-47 torch counts when facing north, in the middle row (tier 2)", lightingWallTorchTier(cells, 0x8000) == 2 &&
                                                                                    lightingWallTorchTier(cells, 0x4000) == 0);
    cells[4] = 0;
    cells[7] = 50;
    check("type 50 faces west, bottom row (tier 3)", lightingWallTorchTier(cells, 0x2000) == 3);
    cells[1] = 48;
    cells[7] = 47;
    check("the first matching cell wins", lightingWallTorchTier(cells, 0x4000) == 1);
}

static void testViewport(void) {
    int16_t grad[LightingGradientSize] = {100, 101, 102, 103, 104, 105, 106};
    int16_t t[LightingViewportCells];
    lightingViewportTable(grad, t);
    check("the centre of the far row is the last delta", t[31] == 106);
    check("its four neighbours are delta 5", t[22] == 105 && t[30] == 105 && t[32] == 105 && t[40] == 105);
    check("the corners keep the base delta 2", t[0] == 102 && t[62] == 102);
    check("band 3 and band 4 spots", t[4] == 103 && t[58] == 103 && t[13] == 104 && t[49] == 104);
    unsigned counts[7] = {0};
    for (unsigned i = 0; i < LightingViewportCells; i++) {
        counts[t[i] - 100]++;
    }
    check("63 cells split 38 / 12 / 8 / 4 / 1 across deltas 2-6", counts[2] == 38 && counts[3] == 12 && counts[4] == 8 && counts[5] == 4 && counts[6] == 1);
}

int main(void) {
    testDayCycle();
    testFlags();
    testLightSources();
    testTorchTier();
    testViewport();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

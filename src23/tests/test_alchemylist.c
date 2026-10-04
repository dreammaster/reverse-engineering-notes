/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_alchemylist test_alchemylist.c ../alchemylist.c ../viewrender.c ../random.c ../pictures.c ../font.c ../worldmap.c && ./test_alchemylist
 */
#include <stdio.h>
#include <string.h>

#include "alchemylist.h"
#include "pictures.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static const uint8_t *frame(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[210 * 105];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)(0x30 + id), sizeof(pixels));
    return pixels;
}

static bool anyColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (screen[y * 320 + x] == colour) {
                return true;
            }
        }
    }
    return false;
}

int main(void) {
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, frame, NULL, screen, NULL};
    AlchemyRow rows[3] = {{"HEAL", 0xF, 3, 0, 0}, {"FIREBALL", 0x25, 9, 5, 2}, {"LIGHT", 0xF, 1, 0, 0}};
    memset(screen, 0xEE, sizeof(screen));
    alchemyListDraw(&r, rows, 3, 2);
    check("the frame is category 1 picture 4 at (15, 23)", screen[23 * 320 + 15] == 0x34 && screen[22 * 320 + 15] == 0xEE && screen[127 * 320 + 224] == 0x34);
    check("row 0's name in its colour at (21, 37)", anyColour(screen, 21, 37, 50, 43, 0xF));
    check("row 1 (6 pixels lower) in colour 0x25 with all three costs", anyColour(screen, 21, 43, 70, 49, 0x25) && anyColour(screen, 150, 43, 160, 49, 0x25) &&
                                                                          anyColour(screen, 179, 43, 190, 49, 0x25) && anyColour(screen, 202, 43, 214, 49, 0x25));
    check("row 0 has no NUORE/ORE cost columns", !anyColour(screen, 179, 37, 190, 43, 0xF) && !anyColour(screen, 202, 37, 214, 43, 0xF) && anyColour(screen, 150, 37, 160, 43, 0xF));
    check("the selected castable row (0xF) is drawn in 0x8A", anyColour(screen, 21, 49, 60, 55, 0x8A) && !anyColour(screen, 21, 49, 60, 55, 0xF));
    memset(screen, 0xEE, sizeof(screen));
    alchemyListDraw(&r, rows, 3, 1);
    check("a selected uncastable row is 0x85", anyColour(screen, 21, 43, 70, 49, 0x85) && !anyColour(screen, 21, 43, 70, 49, 0x25));

    ViewRenderer r3 = {GameYendor3, NULL, frame, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    alchemyListDraw(&r3, rows, 3, -1);
    check("Chapter 3: frame picture 3, MP at x = 170, the second cost at 203, no third column", screen[23 * 320 + 15] == 0x33 && anyColour(screen, 170, 43, 180, 49, 0x25) &&
                                                                                                   anyColour(screen, 203, 43, 214, 49, 0x25) && !anyColour(screen, 150, 43, 160, 49, 0x25) &&
                                                                                                   !anyColour(screen, 215, 43, 230, 49, 0x25));

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

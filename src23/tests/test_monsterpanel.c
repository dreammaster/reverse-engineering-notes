/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monsterpanel test_monsterpanel.c ../monsterpanel.c ../viewrender.c ../random.c ../pictures.c ../font.c ../worldmap.c && ./test_monsterpanel
 */
#include <stdio.h>
#include <string.h>

#include "monsterpanel.h"
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

static const uint8_t *fill(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[64];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)(0x40 + id), sizeof(pixels));
    return pixels;
}

static void put16(uint8_t *m, unsigned offset, unsigned value) {
    m[offset] = (uint8_t)value;
    m[offset + 1] = (uint8_t)(value >> 8);
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
    check("a full bar is 45, half is 22, never below 1", monsterPanelBarWidth(10, 10) == 45 && monsterPanelBarWidth(5, 10) == 22 && monsterPanelBarWidth(1, 1000) == 1 &&
                                                              monsterPanelBarWidth(0, 10) == 4);
    check("the three panel rows", monsterPanelY(0) == 87 && monsterPanelY(1) == 123 && monsterPanelY(2) == 159);

    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, fill, NULL, screen, NULL};
    uint8_t m[156];
    memset(m, 0, sizeof(m));
    memcpy(m + 0x32, "GIANT ANT", 10);
    memcpy(m + 0x3F, "(SOLDIER)", 10);
    put16(m, 0x00, 5);
    put16(m, 0x10, 5);
    put16(m, 0x50, 10);
    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 0, m, false, 0);
    check("names are drawn in colour 9 at (241, 87) and (241, 93)", anyColour(screen, 241, 87, 300, 93, 9) && anyColour(screen, 241, 93, 300, 99, 9));
    check("...and nothing else below tier 55", screen[100 * 320 + 241] == 0xEE);

    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 1, m, true, 55);
    check("the active monster's name is 0x8A", anyColour(screen, 241, 123, 300, 129, 0x8A));
    check("from tier 55 a health bar half full (22 of 45)", screen[135 * 320 + 241] == 0x59 && screen[135 * 320 + 241 + 21] == 0x59 && screen[135 * 320 + 241 + 22] == 6 &&
                                                                screen[142 * 320 + 241] == 0x59 && screen[143 * 320 + 241] == 0xEE);
    check("...no icons yet", screen[135 * 320 + 241 + 46] == 0xEE);

    put16(m, 0x0C, 0x4000 | 0x2000 | 0x800 | 0x20);
    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 2, m, false, 75);
    check("from tier 75 three icons: diseased(7), paralyzed(8) hexed(12)", screen[171 * 320 + 241 + 46] == 0x47 && screen[171 * 320 + 241 + 55] == 0x48 && screen[171 * 320 + 241 + 64] == 0x4C);
    check("a revealed monster's names are 0xAA", anyColour(screen, 241, 159, 300, 165, 0xAA));
    check("the one-shot quality bits are cleared, the others stay", m[0x0C] == (0x4000 | 0x2000 | 0x800) % 256 && m[0x0D] == (0x4000 | 0x2000 | 0x800) >> 8);

    put16(m, 0x0C, 0x40);
    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 0, m, false, 80);
    check("from tier 80 the health quality bit writes HEALTH: and the numbers", anyColour(screen, 241, 108, 300, 114, 9) && anyColour(screen, 241, 114, 300, 120, 9));

    put16(m, 0x10, 50);
    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 0, m, false, 55);
    check("health above the maximum is a full bar, 2 colours brighter", screen[99 * 320 + 241] == 0x5B && screen[99 * 320 + 241 + 44] == 0x5B);

    memset(m, 0, sizeof(m));
    memset(screen, 0xEE, sizeof(screen));
    monsterPanelDraw(&r, 0, m, false, 99);
    check("an empty slot draws nothing", screen[90 * 320 + 241] == 0xEE);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluebook test_cluebook.c ../cluebook.c ../font.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c && ./test_cluebook
 */
#include <stdio.h>
#include <string.h>

#include "cluebook.h"
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

static const uint8_t *tile(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[256];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)id, sizeof(pixels));
    return pixels;
}

int main(void) {
    check("a tab's picture is its base id, +1 when selected (Chapter 2)", clueTabPicture(GameYendor2, 0, 0) == 0x20 && clueTabPicture(GameYendor2, 0, 0x8000) == 0x21 &&
                                                                            clueTabPicture(GameYendor2, 1, 0x4000) == 0x146 && clueTabPicture(GameYendor2, 6, 0x200) == 0x14E &&
                                                                            clueTabPicture(GameYendor2, 3, 0x8000) == 0x153);
    check("Chapter 3 has its own ids", clueTabPicture(GameYendor3, 0, 0) == 0x1E && clueTabPicture(GameYendor3, 6, 0x200) == 0x2B);
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, tile, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    clueNavBarDraw(&r, 0x2000 | 0x40);
    check("seven icons from x = 62, 30 apart, at y = 180", screen[180 * 320 + 62] == 0x20 && screen[195 * 320 + 77] == 0x20 && screen[180 * 320 + 92] == 0x45 && screen[180 * 320 + 242] == 0x4D &&
                                                              screen[180 * 320 + 61] == 0xEE && screen[180 * 320 + 78] == 0xEE);
    check("the selected tab (0x2000 = tab 2) is its +1 picture", screen[180 * 320 + 122] == 0x48);
    bool hint = false;
    for (int y = 185; y < 191; y++) {
        for (int x = 11; x < 50; x++) {
            hint = hint || screen[y * 320 + x] == 0xF;
        }
    }
    check("the LIST hint is drawn, the MAP hint not", hint && screen[186 * 320 + 290] == 0xEE);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

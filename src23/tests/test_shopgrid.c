/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_shopgrid test_shopgrid.c ../shopgrid.c ../viewrender.c ../random.c ../pictures.c ../font.c ../uiregions.c ../item.c ../worldmap.c && ./test_shopgrid
 */
#include <stdio.h>
#include <string.h>

#include "pictures.h"
#include "shopgrid.h"

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
    static uint8_t pixels[256];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)(0x40 + id), sizeof(pixels));
    return pixels;
}

int main(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.itemCount = 10;
    catalog.items[(3 - 1) * ItemRecordSize + ItemFieldIcon] = 21; /* item 3's icon is picture 21 */
    catalog.items[(6 - 1) * ItemRecordSize + ItemFieldIcon] = 30;
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, fill, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    uint16_t ids[8] = {3, 0, 0, 6, 0, 0, 0, 0};
    unsigned drawn = shopGridDraw(&r, &catalog, ids);
    check("two items drawn", drawn == 2);
    check("the area is filled with colour 4", screen[160 * 320 + 241] == 4 + 0 || screen[160 * 320 + 241] == 0x40 + 21);
    check("slot 0's icon is at the first box (241, 160)", screen[160 * 320 + 241] == 0x40 + 21 && screen[175 * 320 + 256] == 0x40 + 21);
    check("slot 3 (the last box of the first row) holds item 6's icon", screen[160 * 320 + 295] == 0x40 + 30);
    check("empty slots stay the background colour", screen[160 * 320 + 259] == 4 && screen[179 * 320 + 241] == 4);
    check("nothing outside the 72 x 35 area is touched", screen[159 * 320 + 241] == 0xEE && screen[160 * 320 + 240] == 0xEE && screen[195 * 320 + 241] == 0xEE);

    memset(screen, 0xEE, sizeof(screen));
    uint16_t none[8] = {0};
    check("no items: zero drawn and EMPTY is written", shopGridDraw(&r, &catalog, none) == 0);
    bool text = false;
    for (int y = 179; y < 185; y++) {
        for (int x = 259; x < 289; x++) {
            text = text || screen[y * 320 + x] == 0xF;
        }
    }
    check("...in colour 0xF", text);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

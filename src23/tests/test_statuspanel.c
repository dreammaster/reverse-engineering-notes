/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_statuspanel test_statuspanel.c ../statuspanel.c ../viewrender.c ../random.c ../pictures.c ../font.c ../uiregions.c ../worldmap.c && ./test_statuspanel
 */
#include <stdio.h>
#include <string.h>

#include "party.h"
#include "pictures.h"
#include "statuspanel.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

/* every picture is filled with (category * 16 + id) & 0x7F so a pixel says which picture put it there */
static const uint8_t *fill(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[318 * 198];
    (void)ctx;
    const PictureCategory *c = pictureCategory(GameYendor2, category);
    memset(pixels, (uint8_t)((category * 16 + id) & 0x7F), (size_t)c->width * c->height);
    return pixels;
}

static void put16(uint8_t *record, unsigned offset, unsigned value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

int main(void) {
    check("a full bar is 38 wide", statusPanelBarWidth(100, 100) == 38 && statusPanelBarWidth(1, 1) == 38);
    check("half a bar is 19", statusPanelBarWidth(50, 100) == 19);
    check("a bar never shows less than one pixel while the value is positive", statusPanelBarWidth(1, 1000) == 1);
    check("nothing at 0 or below", statusPanelBarWidth(0, 100) == 0 && statusPanelBarWidth(-5, 100) == 0);
    check("a value above the maximum is a full bar", statusPanelBarWidth(500, 100) == 38);
    check("the quotient truncates before the division (100 * max / cur = 150 -> 3800 / 150)", statusPanelBarWidth(2, 3) == 25);

    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, fill, NULL, screen, NULL};
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    put16(record, 0x12, 21);
    put16(record, 0x52, 50); /* HP 50 / 100 */
    put16(record, 0x92, 100);
    put16(record, 0x54, 0);
    put16(record, 0x94, 20);
    put16(record, 0x118, 30); /* load 30 / 60 */
    put16(record, 0x56, 60);
    put16(record, 0xB4, 0x8000);
    memset(screen, 0xEE, sizeof(screen));
    statusPanelDraw(&r, 0, record);
    check("the face is category 7 picture [+0x12] at the panel's portrait cell", screen[148 * 320 + 8] == ((7 * 16 + 21) & 0x7F) && screen[179 * 320 + 39] == ((7 * 16 + 21) & 0x7F) &&
                                                                                      screen[147 * 320 + 8] == 0xEE);
    check("the HP bar is half full in colour 0x59 over background 6", screen[181 * 320 + 9] == 0x59 && screen[181 * 320 + 9 + 18] == 0x59 && screen[181 * 320 + 9 + 19] == 6 &&
                                                                         screen[185 * 320 + 9] == 0x59 && screen[186 * 320 + 9] != 0x59);
    check("the MP bar (0 of 20) is empty, 5 rows below", screen[186 * 320 + 9] == 6 && screen[190 * 320 + 9 + 37] == 6);
    check("the load bar is half full in colour 0x86", screen[191 * 320 + 9] == 0x86 && screen[191 * 320 + 9 + 19] == 6);
    check("the abilities icon is picture 0x14 (a nonzero [+0xB4])", screen[170 * 320 + 50] == ((9 * 16 + 0x14) & 0x7F));
    check("the training mark slot is the blank icon with nothing pending", screen[150 * 320 + 50] == ((9 * 16 + 0x15) & 0x7F));
    check("the affliction icons are blank (4)", screen[150 * 320 + 40] == ((9 * 16 + 4) & 0x7F) && screen[160 * 320 + 40] == ((9 * 16 + 4) & 0x7F) &&
                                                  screen[170 * 320 + 40] == ((9 * 16 + 4) & 0x7F));
    check("the protection slot is blank too", screen[160 * 320 + 50] == ((9 * 16 + 0x15) & 0x7F));

    put16(record, 0x1C, 0x2000 | 0x800 | 0x100);
    put16(record, 0x1E, 1);
    put16(record, 0x20, 5);
    memset(screen, 0xEE, sizeof(screen));
    statusPanelDraw(&r, 1, record);
    int dx = 58; /* the second panel is 58 pixels on */
    check("status flags choose the affliction icons: diseased 7, frozen 9, hexed 12", screen[150 * 320 + 40 + dx] == ((9 * 16 + 7) & 0x7F) &&
                                                                                          screen[160 * 320 + 40 + dx] == ((9 * 16 + 9) & 0x7F) &&
                                                                                          screen[170 * 320 + 40 + dx] == ((9 * 16 + 12) & 0x7F));
    check("any protection shows icon 0xE", screen[160 * 320 + 50 + dx] == ((9 * 16 + 0xE) & 0x7F));
    check("a pending level draws a 'T' instead of the blank icon", screen[150 * 320 + 50 + dx] == 0xEE || screen[150 * 320 + 50 + dx] == 0x0F);
    check("a frozen member gets the overlay picture over the face", screen[148 * 320 + 8 + dx] == ((7 * 16 + 0xE0) & 0x7F));

    put16(record, 0x1C, PartyStatusDead);
    memset(screen, 0xEE, sizeof(screen));
    statusPanelDraw(&r, 2, record);
    check("a dead member's HP bar is empty", screen[181 * 320 + 9 + 116] == 6);
    bool anyFifteen = false;
    for (int y = 181; y < 187; y++) {
        for (int x = 9 + 116; x < 9 + 116 + 38; x++) {
            anyFifteen = anyFifteen || screen[y * 320 + x] == 0x0F;
        }
    }
    check("...and 'DEAD' is written over the bars", anyFifteen);

    memset(screen, 0xEE, sizeof(screen));
    statusPanelDraw(&r, 3, NULL);
    check("an empty slot draws nothing", screen[148 * 320 + 8 + 174] == 0xEE);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_charcreate test_charcreate.c ../charcreate.c ../font.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c && ./test_charcreate
 *
 * Face ids 20-33 are what the real Chapter 2 heroes carry.
 */
#include <stdio.h>
#include <string.h>

#include "charcreate.h"
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

static const uint8_t *face(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[32 * 32];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)id, sizeof(pixels));
    return pixels;
}

int main(void) {
    check("male portrait 1: body 0, face 19", charCreateBodyPicture(1, 1) == 0 && charCreateFacePicture(1, 1) == 19);
    check("female portrait 1: body 1, face 20", charCreateBodyPicture(2, 1) == 1 && charCreateFacePicture(2, 1) == 20);
    check("portrait 9: bodies 16 / 17, faces 35 / 36", charCreateBodyPicture(1, 9) == 16 && charCreateBodyPicture(2, 9) == 17 && charCreateFacePicture(1, 9) == 35 &&
                                                              charCreateFacePicture(2, 9) == 36);
    uint8_t record[500];
    memset(record, 0, sizeof(record));
    charCreateChoosePortrait(record, 2, 4);
    check("choosing writes [+0x14] body and [+0x12] face", record[0x14] == 7 && record[0x12] == 7 + 0x13);
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, face, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    charCreatePortraitGridDraw(&r, 1);
    check("the grid: nine faces, row by row, 33 pixels apart from (8, 42)", screen[42 * 320 + 8] == 19 && screen[42 * 320 + 41] == 21 && screen[75 * 320 + 8] == 25 &&
                                                                             screen[108 * 320 + 74] == 35 && screen[41 * 320 + 8] == 0xEE && screen[42 * 320 + 40] == 0xEE);
    memset(screen, 0xEE, sizeof(screen));
    charCreateClassPickDraw(&r);
    bool header = false, hot = false, normal = false, second = false;
    for (int y = 25; y < 31; y++) {
        for (int x = 8; x < 80; x++) {
            header = header || screen[y * 320 + x] == 0x8A;
        }
    }
    for (int y = 51; y < 57; y++) {
        for (int x = 8; x < 14; x++) {
            hot = hot || screen[y * 320 + x] == 0x7B; /* the F of FIGHTER */
        }
        for (int x = 14; x < 50; x++) {
            normal = normal || screen[y * 320 + x] == 0xF;
        }
    }
    for (int y = 105; y < 111; y++) {
        for (int x = 8; x < 20; x++) {
            second = second || screen[y * 320 + x] == 0x7B; /* ALCHEMIST: the A is first */
        }
    }
    check("the class screen: header, FIGHTER with a highlighted F, ALCHEMIST at the second row of the second group", header && hot && normal && second);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

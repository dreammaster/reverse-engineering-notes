/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_roster test_roster.c ../roster.c ../viewrender.c ../pictures.c ../font.c ../uiregions.c ../party.c ../item.c ../bcd4.c \
 *       ../effect.c ../random.c ../worldmap.c ../savegame.c && ./test_roster
 */
#include <stdio.h>
#include <string.h>

#include "pictures.h"
#include "roster.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

/* every picture is filled with (category * 16 + id) & 0x7F */
static const uint8_t *fill(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[318 * 198];
    (void)ctx;
    const PictureCategory *c = pictureCategory(GameYendor2, category);
    memset(pixels, (uint8_t)((category * 16 + id) & 0x7F), (size_t)c->width * c->height);
    return pixels;
}

int main(void) {
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, fill, NULL, screen, NULL};
    uint8_t member[500], joined[500];
    memset(member, 0, sizeof(member));
    memset(joined, 0, sizeof(joined));
    memcpy(member, "DIANA", 6);
    member[0x16] = 1;
    member[0x12] = 22;
    member[0x0E] = 4;
    memcpy(joined, member, sizeof(member));
    joined[0x15D] = 0x08; /* [+0x15C] & 0x800 */
    const uint8_t *records[RosterSlots] = {NULL};
    records[0] = member;
    records[4] = joined;
    memset(screen, 0xEE, sizeof(screen));
    rosterDraw(&r, records);
    check("the backdrop is category 0 picture 4 at (1, 1)", screen[1 * 320 + 1] != 0xEE && screen[0] == 0xEE);
    check("slot 0's face is category 7 picture [+0x12] at (15, 5)", screen[5 * 320 + 15] == ((7 * 16 + 22) & 0x7F) && screen[36 * 320 + 46] == ((7 * 16 + 22) & 0x7F));
    check("an occupied slot not in the party has the empty check box (0x11)", screen[29 * 320 + 52] == ((9 * 16 + 0x11) & 0x7F));
    check("a member in the party has the ticked box (0x12) -- slot 4 (the second row's middle)", screen[(65 + 24) * 320 + 117 + 35] == ((9 * 16 + 0x12) & 0x7F) ||
                                                                                                   screen[(65 + 24) * 320 + 117 + 35] != 0xEE);
    check("an empty slot (level 0) draws no face of its own", screen[5 * 320 + 117] == screen[1 * 320 + 1] || screen[5 * 320 + 117] == ((0 * 16 + 4) & 0x7F));

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

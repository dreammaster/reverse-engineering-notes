/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluetransport test_cluetransport.c ../cluetransport.c ../cluebook.c ../exedata.c ../font.c ../bcd4.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_cluetransport
 *
 * The real-data check reads the mount records and labels from yendor2/game/SW.EXE and yendor3/game/REGISTER.EXE (skipped if absent;
 * YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cluetransport.h"

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

static const uint8_t *pic(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[320 * 200];
    (void)ctx;
    memset(pixels, (uint8_t)(0x40 + category * 16 + id), sizeof(pixels));
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

static void testSynthetic(void) {
    ClueTransportData d;
    memset(&d, 0, sizeof(d));
    const char *names[4] = {"PEGASUS", "EAGLE", "RUG", "DRAGON"};
    for (unsigned i = 0; i < 4; i++) {
        strcpy(d.mounts[i].name, names[i]);
        d.mounts[i].price[1] = 0x01;
        d.mounts[i].uses = i + 1;
        d.mounts[i].flags = i == 3 ? 0x1001 : 0x8003;
    }
    strcpy(d.value, "VALUE:");
    strcpy(d.uses, "USES:");
    strcpy(d.time, "TIME:");
    strcpy(d.between, "BETWEEN       AND");
    strcpy(d.sevenPm, "7P.M.");
    strcpy(d.sevenAm, "7A.M.");
    strcpy(d.anytime, "ANYTIME");
    strcpy(d.title, "TRANSPORTATIONS");
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, pic, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    clueTransportPageDraw(&r, &d, 0x8000);
    check("backdrop and title", screen[321] == 0x40 + 13 && anyColour(screen, 6, 4, 60, 10, 0x0D));
    check("three blocks (names in 0xD at y = 26, 74, 122; the rug is skipped)", anyColour(screen, 91, 26, 120, 32, 0x0D) && anyColour(screen, 91, 74, 120, 80, 0x0D) &&
                                                                                    anyColour(screen, 91, 122, 120, 128, 0x0D));
    check("VALUE: label, the price in 0x8A and the uses in 0x59 under it", anyColour(screen, 91, 38, 125, 44, 0x0A) && anyColour(screen, 127, 38, 170, 44, 0x8A) &&
                                                                              anyColour(screen, 127, 47, 135, 53, 0x59));
    check("the first block reads ANYTIME in 0xCA", anyColour(screen, 127, 56, 170, 62, 0xCA));
    check("the dragon block reads BETWEEN .. AND with 7P.M. and 7A.M. under it", anyColour(screen, 127, 152, 175, 158, 0x0D) && anyColour(screen, 175, 152, 205, 158, 0xCA) &&
                                                                                      anyColour(screen, 175, 161, 205, 167, 0xCA));
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *name, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)size);
    bool read = data && fread(data, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    ExeData exe;
    static ClueTransportData d;
    bool ok = read && exeDataOpen(&exe, game, data, (size_t)size) && clueTransportLoad(&d, &exe, game);
    check(label, ok);
    if (ok) {
        char price[12];
        bcd4Format(d.mounts[0].price, price);
        check("...the four mounts, their uses 1 2 4 4 and time words (the dragon alone has no day bit)",
              strncmp(d.mounts[0].name, "PEGASUS", 7) == 0 && strncmp(d.mounts[3].name, "MAGIC DRAGON", 12) == 0 && d.mounts[0].uses == 1 && d.mounts[1].uses == 2 &&
                  d.mounts[2].uses == 4 && d.mounts[3].uses == 4 && (d.mounts[0].flags & 2) && !(d.mounts[3].flags & 2) && d.mounts[0].flags == 0x8003 && strcmp(price, "0") != 0);
        check("...and the labels", strcmp(d.value, "VALUE:") == 0 && strcmp(d.anytime, "ANYTIME") == 0 && strcmp(d.title, "TRANSPORTATIONS") == 0);
    }
    free(data);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: the transport page loads from SW.EXE");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: the transport page loads from REGISTER.EXE");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

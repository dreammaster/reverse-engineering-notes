/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluepaged test_cluepaged.c ../cluepaged.c ../cluebook.c ../exedata.c ../font.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_cluepaged
 *
 * The real-data checks read the executables and WORLD.DAT in yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cluepaged.h"

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

static uint8_t *slurp(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)n);
    if (data && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return data;
}

static void testLogic(void) {
    unsigned page = 1;
    check("page 1 has only a next page, the last only a previous one", cluePagedNavFlags(GameYendor2, 1) == 0x80 && cluePagedNavFlags(GameYendor2, 31) == 0x100 &&
                                                                         cluePagedNavFlags(GameYendor3, 33) == 0x100 && cluePagedNavFlags(GameYendor2, 5) == 0x180);
    check("I on page 1 does nothing, Q goes on", cluePagedNavigate(GameYendor2, &page, 'I', 0, true) == CluePagedNone && page == 1 &&
                                                    cluePagedNavigate(GameYendor2, &page, 'Q', 0, true) == CluePagedNext && page == 2);
    check("clicks: region 1 back, region 2 on", cluePagedNavigate(GameYendor2, &page, 0, 1, true) == CluePagedPrevious && page == 1 &&
                                                   cluePagedNavigate(GameYendor2, &page, 0, 2, true) == CluePagedNext && page == 2);
    page = 5;
    check("unregistered: page 5 -> 6 is allowed, 6 -> 7 shows the nag", cluePagedNavigate(GameYendor2, &page, 'Q', 0, false) == CluePagedNext && page == 6 &&
                                                                          cluePagedNavigate(GameYendor2, &page, 'Q', 0, false) == CluePagedNag && page == 6);
    check("registered it goes on", cluePagedNavigate(GameYendor2, &page, 'Q', 0, true) == CluePagedNext && page == 7);
    page = 31;
    check("nothing after the last page", cluePagedNavigate(GameYendor2, &page, 'Q', 0, true) == CluePagedNone && page == 31);
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *exeName, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, exeName);
    size_t exeSize, worldSize;
    uint8_t *exeData = slurp(path, &exeSize);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    uint8_t *world = slurp(path, &worldSize);
    if (!exeData || !world) {
        printf("SKIP %s (game files not found)\n", label);
        g_skipCount++;
        free(exeData);
        free(world);
        return;
    }
    ExeData exe;
    CluePagedText t;
    check(label, exeDataOpen(&exe, game, exeData, exeSize) && cluePagedTextLoad(&t, &exe, game) && strcmp(t.heading, "COMPLETE WALK THROUGH") == 0 && strncmp(t.footer, "a  MORE  b", 10) == 0);
    bool pagesOk = true;
    for (unsigned p = 1; p <= cluePageCount(game); p++) {
        const uint8_t *data = cluePagedPage(game, world, worldSize, p);
        pagesOk = pagesOk && data && data[0] >= 0x20 && data[0] < 0x7F;
    }
    check("...every page is there and starts with text", pagesOk && !cluePagedPage(game, world, worldSize, cluePageCount(game) + 1) && !cluePagedPage(game, world, worldSize, 0));
    static uint8_t screen[320 * 200];
    ViewRenderer r = {game, NULL, pic, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    cluePagedDraw(&r, &t, cluePagedPage(game, world, worldSize, 1), 1, cluePagedNavFlags(game, 1));
    bool lines = false;
    for (int y = 23; y < 29; y++) {
        for (int x = 10; x < 120; x++) {
            lines = lines || screen[y * 320 + x] == 0x0D;
        }
    }
    check("...and page 1 draws its first line in 0xD at (10, 23)", lines);
    static ClueHelpText h;
    check("...the help screen text loads (13 lines, the nag reminder)", clueHelpTextLoad(&h, &exe, game) && strstr(h.banner, "PRESS TAB") && strstr(h.lines[0], "F1") && strstr(h.nag, "REGISTER YOUR COPY"));
    memset(screen, 0xEE, sizeof(screen));
    clueHelpDraw(&r, &h, 0x8000 | 0x60);
    clueNagDraw(&r, &h);
    bool banner = false, nag = false;
    for (int y = 24; y < 30; y++) {
        for (int x = 21; x < 60; x++) {
            banner = banner || screen[y * 320 + x] == 0x59;
        }
    }
    for (int y = 16; y < 22; y++) {
        for (int x = 35; x < 80; x++) {
            nag = nag || screen[y * 320 + x] == 0x59;
        }
    }
    check("...and draws the banner and the nag in 0x59", banner && nag);
    free(exeData);
    free(world);
}

int main(void) {
    testLogic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: the walk through text loads");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: the walk through text loads");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

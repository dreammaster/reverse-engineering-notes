/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_paperdoll test_paperdoll.c ../paperdoll.c ../viewrender.c ../pictures.c ../pictures_stdio.c ../uiregions.c \
 *       ../worldmap.c ../item.c ../newgame.c ../savegame.c ../party.c ../bcd4.c ../effect.c ../random.c && ./test_paperdoll
 *
 * Draws the four new-game heroes' paper dolls from the real WORLD.DAT / PICTURES.VGA of yendor2/game and yendor3/game (skipped if
 * absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override) and compares a hash with one recorded after inspecting the render.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "newgame.h"
#include "paperdoll.h"
#include "pictures_stdio.h"

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

static uint32_t fnv(const uint8_t *p, size_t n) {
    uint32_t h = 2166136261u;
    while (n--) {
        h = (h ^ *p++) * 16777619u;
    }
    return h;
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, uint32_t expected, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    static uint8_t world[5000000];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    size_t size = f ? fread(world, 1, sizeof(world), f) : 0;
    if (f) {
        fclose(f);
    }
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir ? dir : defaultDir);
    PictureFile *pictures = size ? pictureFileOpen(path, game) : NULL;
    if (!pictures) {
        printf("SKIP %s (game files not found)\n", label);
        g_skipCount++;
        return;
    }
    static ItemCatalog items;
    static SaveGame save;
    saveGameInit(&save, game);
    bool ok = itemCatalogParseWorldDat(&items, game, world, size) && saveGameNewGame(&save, game, world, size);
    check("the catalog and the new-game template load", ok);
    static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    memset(screen, 0, sizeof(screen));
    ViewRenderer r = {game, NULL, pictureFileGet, pictures, screen, NULL};
    for (unsigned i = 0; i < 4; i++) {
        paperDollDraw(&r, &items, saveGamePartyRecord(&save, 5 + i), 8 + 56 * (int)i, 8);
    }
    paperDollDraw(&r, &items, NULL, 0, 0);
    pictureFileClose(pictures);
    uint32_t h = fnv(screen, sizeof(screen));
    if (h != expected) {
        printf("(frame hash is 0x%08X)\n", h);
    }
    check(label, h == expected);
}

int main(void) {
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", 0xC15F295E, "Chapter 2: the four heroes' paper dolls (inspected render)");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", 0x0BE226AB, "Chapter 3: the four heroes' paper dolls (inspected render)");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

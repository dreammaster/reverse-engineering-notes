/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_location test_location.c ../location.c && ./test_location
 *
 * The real-data checks read WORLD.DAT in yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "location.h"

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

static void testSynthetic(void) {
    static uint8_t world[600000];
    memset(world, 0, sizeof(world));
    memcpy(world + 461520 + 20 * 3, "SOME PLACE          ", 20);
    memcpy(world + 460800 + 6 * 5, "1   \x03\x02", 6);  /* level 1 */
    memcpy(world + 460800 + 6 * 6, "03  \x03\x02", 6);  /* map 3 */
    memcpy(world + 460800 + 6 * 7, "0   \x03\x02", 6);  /* nothing */
    LocationName n;
    check("a level block", locationName(GameYendor2, world, sizeof(world), 5, " LEVEL X", " MAP X", &n) && strcmp(n.name, "SOME PLACE") == 0 && strcmp(n.suffix, " LEVEL 1") == 0 && n.kind == 2);
    check("a map block (character 1)", locationName(GameYendor2, world, sizeof(world), 6, " LEVEL X", " MAP X", &n) && strcmp(n.suffix, " MAP 3") == 0 && n.kind == 1);
    check("no suffix", locationName(GameYendor2, world, sizeof(world), 7, " LEVEL X", " MAP X", &n) && n.kind == 0 && n.suffix[0] == 0);
    check("out of range", !locationName(GameYendor2, world, sizeof(world), 120, " LEVEL X", " MAP X", &n) && !locationName(GameYendor2, world, 100, 0, "", "", &n));
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
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
    const char *level = game == GameYendor2 ? " LEVEL X" : " LEVEL XXX", *map = game == GameYendor2 ? " MAP X" : " MAP XXX";
    unsigned blocks = game == GameYendor2 ? 120 : 140, named = 0, suffixed = 0;
    LocationName n;
    for (unsigned b = 0; read && b < blocks; b++) {
        if (locationName(game, data, (size_t)size, b, level, map, &n) && n.name[0]) {
            named++;
            suffixed += n.kind != 0;
        }
    }
    check(label, read && named > 30 && suffixed > 10);
    if (read && game == GameYendor2) {
        check("...block 21 of Chapter 2 is a map of the Yendor-Port Hope area", locationName(game, data, (size_t)size, 21, level, map, &n) && strcmp(n.name, "YENDOR-PORT HOPE") == 0 && strcmp(n.suffix, " MAP 1") == 0);
    }
    free(data);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: the block names read from WORLD.DAT");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: the block names read from WORLD.DAT");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

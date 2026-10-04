/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluemap test_cluemap.c ../cluemap.c ../localmap.c ../location.c ../minimap.c ../explore.c ../savegame.c ../worldmap.c ../worldmap_stdio.c ../font.c ../viewrender.c ../random.c ../pictures.c ../uiregions.c ../dungeongrid.c ../movement.c && ./test_cluemap
 *
 * The real-data checks read WORLD.DAT in yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cluemap.h"
#include "worldmap_stdio.h"

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
    ClueMapMarker markers[4] = {{45, 26, 7, 40, 24}, {100, 30, 1, 160, 56}};
    check("a click inside a marker's 8 x 8 square hits it, outside misses", clueMapMarkerAt(markers, 2, 44, 28) == 0 && clueMapMarkerAt(markers, 2, 160, 64) == 1 &&
                                                                              clueMapMarkerAt(markers, 2, 49, 28) == -1 && clueMapMarkerAt(markers, 2, 0, 0) == -1);
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
    static WorldMap map;
    bool haveMap = read && worldMapReadWorldDatFile(&map, game, path);
    unsigned total = 0, maxPerMap = 0, mapsWithMarkers = 0;
    ClueMapMarker markers[ClueMapMarkersMax];
    unsigned blocks = game == GameYendor2 ? 120 : 140;
    for (unsigned id = 1; read && id <= blocks; id++) {
        unsigned n = clueMapMarkers(game, data, (size_t)size, id, markers, ClueMapMarkersMax);
        total += n;
        maxPerMap = n > maxPerMap ? n : maxPerMap;
        mapsWithMarkers += n > 0;
    }
    check(label, haveMap && total > 100 && mapsWithMarkers > 5 && maxPerMap < ClueMapMarkersMax);
    if (haveMap) {
        char text[ClueMapLabelSize];
        unsigned n = clueMapMarkers(game, data, (size_t)size, game == GameYendor2 ? 22 : 24, markers, ClueMapMarkersMax);
        bool labelsOk = n > 0;
        for (unsigned i = 0; i < n; i++) {
            labelsOk = labelsOk && clueMapLabel(game, data, (size_t)size, markers[i].label, text) && text[0] > ' ' && text[0] < 0x7F;
        }
        check("...the markers of a town map have legend labels and lie inside the block", labelsOk && markers[0].x >= (game == GameYendor2 ? 40u : 0u));
        static LocalMapCell cells[LocalMapColumns * LocalMapRows];
        clueMapFill(cells, game, &map, data, (size_t)size, game == GameYendor2 ? 22 : 24);
        unsigned known = 0;
        for (unsigned i = 0; i < LocalMapColumns * LocalMapRows; i++) {
            known += cells[i].explored;
        }
        check("...and the known bitmap marks a good part of the block", known > 100);
    }
    free(data);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: the clue map markers read from WORLD.DAT");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: the clue map markers read from WORLD.DAT");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

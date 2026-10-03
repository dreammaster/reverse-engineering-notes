/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_viewrender test_viewrender.c ../viewrender.c ../viewport.c ../pictures.c ../pictures_stdio.c \
 *       ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_viewrender
 *
 * The real-data check renders views from WORLD.DAT / PICTURES.VGA in yendor2/game and yendor3/game (skipped if absent;
 * YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override) and compares a hash of the frame with one recorded after inspecting the render.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lighting.h"
#include "pictures_stdio.h"
#include "viewport.h"
#include "viewrender.h"
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

static void testShade(void) {
    check("a zero delta leaves the colour", viewShadeColour(0x57, 0) == 0x57);
    check("colours from 0xD0 on are never shaded", viewShadeColour(0xD3, -2) == 0xD3 && viewShadeColour(0xFF, -5) == 0xFF);
    check("a negative delta darkens inside the 16-colour block", viewShadeColour(0x57, -2) == 0x55);
    check("...and clamps at the block's first colour", viewShadeColour(0x52, -9) == 0x50 && viewShadeColour(0x50, -1) == 0x50);
    check("a positive delta lightens and clamps at the last", viewShadeColour(0x57, 3) == 0x5A && viewShadeColour(0x5E, 4) == 0x5F);
    check("the high half of the palette clamps the same way", viewShadeColour(0x9E, 5) == 0x9F && viewShadeColour(0x91, -4) == 0x90);
}

/* ---- synthetic tables ---- */

static uint8_t g_tables[ViewTablesSize];
static uint8_t g_screen[ViewScreenWidth * ViewScreenHeight];
static uint8_t g_picture[224 * 136];

static void w16(unsigned offset, unsigned value) {
    g_tables[offset] = (uint8_t)value;
    g_tables[offset + 1] = (uint8_t)(value >> 8);
}

static void words(unsigned offset, const unsigned *list, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        w16(offset + 2 * i, list[i]);
    }
}

static const uint8_t *rampPicture(void *ctx, unsigned category, unsigned id) {
    (void)ctx;
    (void)id;
    const PictureCategory *c = pictureCategory(GameYendor2, category);
    for (unsigned i = 0; i < (unsigned)c->width * c->height; i++) {
        g_picture[i] = (uint8_t)(i % 0xC0);
    }
    return g_picture;
}

static uint8_t srcPixel(unsigned category, unsigned x, unsigned y) {
    const PictureCategory *c = pictureCategory(GameYendor2, category);
    return (uint8_t)((y * c->width + x) % 0xC0);
}

static ViewRenderer renderer(void) {
    memset(g_screen, 0, sizeof(g_screen));
    return (ViewRenderer){GameYendor2, g_tables, rampPicture, NULL, g_screen};
}

static void testRowMask(void) {
    memset(g_tables, 0, sizeof(g_tables));
    /* val11 entry for cell 2: x=10, y=5, ptr=0x300; ptr -> {groups 0x320, runs (2,1,1), 0}; groups (2,1,1), 0 */
    w16(0 + 6 * 2, 10);
    w16(0 + 6 * 2 + 2, 5);
    w16(0 + 6 * 2 + 4, 0x300);
    unsigned head[] = {0x320, 2, 1, 1, 0};
    words(0x300, head, 5);
    unsigned groups[] = {2, 1, 1, 0};
    words(0x320, groups, 4);
    ViewRenderer r = renderer();
    viewDrawSprite(&r, 0, 1, 7, 2, 0, true, 0);
    check("row-mask: the first scanline takes every other source pixel", g_screen[5 * 320 + 10] == srcPixel(1, 0, 0) &&
                                                                             g_screen[5 * 320 + 11] == srcPixel(1, 2, 0) && g_screen[5 * 320 + 12] == 0);
    check("...the second drawn row is source row 2 (a row was skipped)", g_screen[6 * 320 + 10] == srcPixel(1, 0, 2) && g_screen[6 * 320 + 11] == srcPixel(1, 2, 2));
    check("...and nothing else was drawn", g_screen[7 * 320 + 10] == 0 && g_screen[4 * 320 + 10] == 0);
    memset(g_screen, 0, sizeof(g_screen));
    viewDrawSprite(&r, 0, 1, 7, 3, 0, true, 0);
    check("an entry with x = 0 draws nothing", g_screen[5 * 320 + 10] == 0);
    memset(g_screen, 0, sizeof(g_screen));
    viewDrawSprite(&r, 0, 1, 7, 2, -1, true, 0);
    check("the shade delta darkens each drawn pixel", g_screen[5 * 320 + 10] == viewShadeColour(srcPixel(1, 0, 0), -1));
}

static void testPatch(void) {
    memset(g_tables, 0, sizeof(g_tables));
    /* val12 (0x386) entry for cell 1: source (20, 3); records {run 3, shift 1}, {run 2, shift -2}, {run 1, shift 0}, 0 */
    w16(0x386 + 6, 20);
    w16(0x386 + 6 + 2, 3);
    w16(0x386 + 6 + 4, 0x500);
    unsigned list[] = {3, 1, 0, 2, 0xFFFE, 0, 1, 0, 0, 0};
    words(0x500, list, 10);
    ViewRenderer r = renderer();
    viewDrawSprite(&r, 1, 4, 0, 1, 0, false, 0);
    unsigned dx = 20 + 8, dy = 3 + 70;
    check("patch: the first run lands at (x + 8, y + 70)", g_screen[dy * 320 + dx] == srcPixel(4, 20, 3) && g_screen[dy * 320 + dx + 2] == srcPixel(4, 22, 3) &&
                                                               g_screen[dy * 320 + dx + 3] == 0);
    check("...the next row shifts one pixel along on both sides", g_screen[(dy + 1) * 320 + dx + 1] == srcPixel(4, 21, 4) &&
                                                                       g_screen[(dy + 1) * 320 + dx] == 0 && g_screen[(dy + 1) * 320 + dx + 2] == srcPixel(4, 22, 4));
    check("...and a negative shift moves back (signed 16-bit)", g_screen[(dy + 2) * 320 + dx - 1] == srcPixel(4, 19, 5) && g_screen[(dy + 2) * 320 + dx] == 0);
    memset(g_screen, 0, sizeof(g_screen));
    w16(0xBF2 + 6, 20);
    w16(0xBF2 + 6 + 2, 3);
    w16(0xBF2 + 6 + 4, 0x500);
    viewDrawSprite(&r, 2, 5, 0, 1, 0, false, 0);
    check("a ceiling patch lands at (x + 8, y + 8)", g_screen[(3 + 8) * 320 + 28] == srcPixel(5, 20, 3));
}

static void testColumns(void) {
    memset(g_tables, 0, sizeof(g_tables));
    /* val14 (0x13B6) entry for cell 1: x=20, y=30; header {2 columns, advance 0} + run record (1, 2, 0) + terminator + empty header */
    w16(0x13B6 + 6, 20);
    w16(0x13B6 + 6 + 2, 30);
    w16(0x13B6 + 6 + 4, 0x400);
    unsigned head[] = {2, 0, 1, 2, 0, 0};
    words(0x400, head, 6);
    ViewRenderer r = renderer();
    viewDrawSprite(&r, 3, 1, 1, 1, 0, true, 0);
    check("columns: each column draws down from the start", g_screen[30 * 320 + 20] == srcPixel(1, 0, 0) && g_screen[31 * 320 + 20] == srcPixel(1, 0, 1) &&
                                                                 g_screen[32 * 320 + 20] == 0);
    check("...the second column samples the next source column", g_screen[30 * 320 + 21] == srcPixel(1, 1, 0) && g_screen[31 * 320 + 21] == srcPixel(1, 1, 1));
}

static void testStrip(void) {
    memset(g_tables, 0, sizeof(g_tables));
    w16(0x13B6 + 6 * 48, 8);
    w16(0x13B6 + 6 * 48 + 2, 20);
    ViewRenderer r = renderer();
    viewDrawSprite(&r, 6, 6, 3, 48, 0, true, 14);
    check("strip: 7 pixels from the frame's column, 113 rows", g_screen[20 * 320 + 8] == srcPixel(6, 14, 0) && g_screen[20 * 320 + 14] == srcPixel(6, 20, 0) &&
                                                                   g_screen[20 * 320 + 15] == 0 && g_screen[(20 + 112) * 320 + 8] == srcPixel(6, 14, 112) &&
                                                                   g_screen[(20 + 113) * 320 + 8] == 0);
}

/* ---- real data ---- */

static uint32_t fnv(const uint8_t *p, size_t n) {
    uint32_t h = 2166136261u;
    while (n--) {
        h = (h ^ *p++) * 16777619u;
    }
    return h;
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, int x, int y, uint16_t facing, unsigned clock, uint32_t expected,
                     const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    static WorldMap map;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    static uint8_t tables[ViewTablesSize];
    bool ok = f && fseek(f, (long)viewTablesOffset(game), SEEK_SET) == 0 && fread(tables, 1, sizeof(tables), f) == sizeof(tables);
    if (f) {
        fclose(f);
    }
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir ? dir : defaultDir);
    PictureFile *pictures = ok ? pictureFileOpen(path, game) : NULL;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    if (!pictures || !worldMapReadWorldDatFile(&map, game, path)) {
        printf("SKIP %s (game files not found)\n", label);
        g_skipCount++;
        pictureFileClose(pictures);
        return;
    }
    static DungeonGrid grid;
    dungeonGridBuild(&grid, game, &map, NULL, x, y);
    DungeonGridCell cells[ViewportCellCount];
    viewportBuild(&grid, facing, x, y, cells);
    viewportComputeVisibility(game, cells);
    ViewScene scene;
    LightingInput light = {0, 0, (uint16_t)clock, facing};
    bool reset;
    lightingComputeGradient(game, &light, 0, scene.gradient, &reset);
    scene.cells = cells;
    scene.facing = facing;
    static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    memset(screen, 0, sizeof(screen));
    ViewRenderer r = {game, tables, pictureFileGet, pictures, screen};
    viewRender(&r, &scene);
    pictureFileClose(pictures);
    uint32_t h = fnv(screen, sizeof(screen));
    if (h != expected) {
        printf("(frame hash is 0x%08X)\n", h);
    }
    check(label, h == expected);
    unsigned outside = 0;
    for (unsigned yy = 0; yy < ViewScreenHeight; yy++) {
        for (unsigned xx = 0; xx < ViewScreenWidth; xx++) {
            if ((yy < 8 || yy >= 144 || xx < 8 || xx >= 232) && screen[yy * ViewScreenWidth + xx] != 0) {
                outside++;
            }
        }
    }
    check("...and nothing is drawn outside the 224 x 136 viewport", outside == 0);
}

int main(void) {
    testShade();
    testRowMask();
    testPatch();
    testColumns();
    testStrip();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", 166, 36, SaveFacingWest, 720, 0x3C32DEEF, "Chapter 2: the inn, facing west by day (inspected render)");
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", 166, 36, SaveFacingWest, 1200, 0x7898374D, "Chapter 2: the same view at night");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", 166, 36, SaveFacingWest, 720, 0x04C83A87, "Chapter 3: a log wall by day (inspected render)");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", 460, 46, SaveFacingNorth, 540, 0x9FC5F5C8, "Chapter 3: the castle gate and its cobbled approach (inspected render)");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

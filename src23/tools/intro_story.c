/*
 * Plays Chapter 2's story cinematic (or, with "ending", its ending) (introstory.h) headlessly and writes a contact sheet of stills: one after every card, picture and cell frame that follows
 * a fade, taken with the palette as it is at that moment (the sheet is a 3-3-2 colour approximation, since a PNG here has one palette).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o intro_story intro_story.c ../introstory.c ../intro2.c ../palettefade.c ../palette.c ../exedata.c ../font.c ../pictures.c ../pictures_stdio.c ../pcx.c
 *   ./intro_story <game dir> <out.png> [sound effects on: 0|1] [all: the opening too | ending: the ending cinematic instead | opening3: Chapter 3 (pass its game dir) | credits: Chapter 2's credits | credits3: Chapter 3's (pass its game dir) | splash / splash3: the boot title]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "introstory.h"
#include "pictures.h"
#include "pictures_stdio.h"
#include "pngwrite.h"

enum { MaxStills = 64, Columns = 4 };

static uint8_t g_still[MaxStills][320 * 200];
static unsigned g_stills;
static uint8_t g_lastScreen[320 * 200], g_lastDac[768];
static unsigned g_ticks, g_sounds;

static void present(void *ctx, const uint8_t *screen, const uint8_t *dac) {
    (void)ctx;
    memcpy(g_lastScreen, screen, sizeof(g_lastScreen));
    memcpy(g_lastDac, dac, sizeof(g_lastDac));
}

static const char *g_logoPath;

/* Chapter 3's 640 x 480 boot logo: written beside the sheet as <out>.logo.png. */
static void hires(void *ctx, const uint8_t *pixels, unsigned width, unsigned height, const uint8_t *palette) {
    (void)ctx;
    char path[600];
    snprintf(path, sizeof(path), "%s.logo.png", g_logoPath);
    writePng(path, pixels, palette, width, height, 1);
}

static void snapshot(void);
static unsigned g_wanted[MaxStills], g_wantedCount;

static void tickFn(void *ctx) {
    (void)ctx;
    g_ticks++;
    for (unsigned i = 0; i < g_wantedCount; i++) { /* INTRO_TICKS=n,n,...: a still at those ticks instead of the per-stage ones */
        if (g_wanted[i] == g_ticks) {
            snapshot();
        }
    }
}

static void soundFn(void *ctx, unsigned id) {
    (void)ctx;
    (void)id;
    g_sounds++;
}

/* a snapshot at every Escape poll that follows a card, picture, frame or fade: the stage is complete there */
static bool g_drawn, g_ending, g_chapter3, g_credits, g_splash;

static void snapshot(void) {
    if (g_stills >= MaxStills) {
        return;
    }
    uint8_t *dst = g_still[g_stills++];
    for (unsigned i = 0; i < 320 * 200; i++) {
        const uint8_t *rgb = g_lastDac + g_lastScreen[i] * 3;
        unsigned r = rgb[0] * 255 / 63, g = rgb[1] * 255 / 63, b = rgb[2] * 255 / 63;
        dst[i] = (uint8_t)((r >> 5) << 5 | (g >> 5) << 2 | (b >> 6));
    }
}

static void mark(void *ctx, unsigned index, const IntroOp *op) {
    (void)ctx;
    if (op->kind == IntroOpPicture || op->kind == IntroOpPictureOpaque || op->kind == IntroOpWipeRight || op->kind == IntroOpZoom || op->kind == IntroOpFrames || op->kind == IntroOpPlainText) {
        g_drawn = true;
    }
    if (op->kind == IntroOpCard || op->kind == IntroOpPicture || op->kind == IntroOpDrawFrame || op->kind == IntroOpFade16Up || op->kind == IntroOpFade || op->kind == IntroOpFadeFrames) {
        g_drawn = true;
    }
    bool shoot = g_wantedCount ? false : g_chapter3 ? (op->kind == IntroOpPoll && g_drawn) || op->kind == IntroOpStory3 || op->kind == IntroOpFlip : g_ending ? (op->kind == IntroOpWait && op->a >= 20) || op->kind == IntroOpFadeOutAll : (g_drawn && op->kind == IntroOpPoll);
    if (shoot && g_stills < MaxStills) {
        printf("still %u at op %u\n", g_stills, index);
        snapshot();
        g_drawn = false;
    }
}

static bool noEscape(void *ctx) {
    (void)ctx;
    return false;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <game dir> <out.png> [sound 0|1]\n", argv[0]);
        return 2;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", argv[1]);
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t worldSize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *world = malloc(worldSize);
    if (!world || fread(world, 1, worldSize, f) != worldSize) {
        return 1;
    }
    fclose(f);
    g_logoPath = argv[2];
    g_chapter3 = argc > 4 && (strcmp(argv[4], "opening3") == 0 || strcmp(argv[4], "credits3") == 0 || strcmp(argv[4], "splash3") == 0);
    g_splash = argc > 4 && (strcmp(argv[4], "splash") == 0 || strcmp(argv[4], "splash3") == 0);
    g_credits = argc > 4 && (strcmp(argv[4], "credits") == 0 || strcmp(argv[4], "credits3") == 0);
    snprintf(path, sizeof(path), "%s/%s", argv[1], g_chapter3 ? "REGISTER.EXE" : "SW.EXE");
    f = fopen(path, "rb");
    if (!f) {
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t exeSize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *exeBytes = malloc(exeSize);
    if (!exeBytes || fread(exeBytes, 1, exeSize, f) != exeSize) {
        return 1;
    }
    fclose(f);
    ExeData exe;
    if (!exeDataOpen(&exe, g_chapter3 ? GameYendor3 : GameYendor2, exeBytes, exeSize)) {
        fprintf(stderr, "not SW.EXE\n");
        return 1;
    }
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", argv[1]);
    PictureFile *pictures = pictureFileOpen(path, g_chapter3 ? GameYendor3 : GameYendor2);
    if (!pictures) {
        return 1;
    }
    if (getenv("INTRO_TICKS")) {
        for (const char *p = getenv("INTRO_TICKS"); *p && g_wantedCount < MaxStills;) {
            g_wanted[g_wantedCount++] = (unsigned)strtoul(p, (char **)&p, 10);
            if (*p == ',') {
                p++;
            }
        }
    }
    IntroAssets assets = {world, worldSize, &exe, pictureFileGet, pictures, g_chapter3 ? GameYendor3 : GameYendor2};
    static IntroStory story;
    bool all = argc > 4 && strcmp(argv[4], "all") == 0;
    g_ending = argc > 4 && strcmp(argv[4], "ending") == 0;
    if (!(g_splash ? introSplashStart(&story, &assets) : g_credits ? introCreditsStart(&story, &assets) : g_chapter3 ? introOpening3Start(&story, &assets) : g_ending ? introEndingStart(&story, &assets) : all ? introOpeningStart(&story, &assets) : introStoryStart(&story, &assets))) {
        fprintf(stderr, "cannot start the story\n");
        return 1;
    }
    IntroHost host = {NULL, present, soundFn, NULL, tickFn, noEscape, mark, argc > 3 && atoi(argv[3]) != 0, NULL, NULL, hires};
    bool finished = g_splash ? introSplashPlay(&story, &assets, &host) : g_credits ? introCreditsPlay(&story, &assets, &host) : g_chapter3 ? introOpening3Play(&story, &assets, &host) : g_ending ? introEndingPlay(&story, &assets, &host) : all ? introPlayAll(&story, &assets, &host) : introStoryPlay(&story, &assets, &host);
    printf("story %s: %u ticks, %u sound events, %u stills\n", finished ? "finished" : "stopped", g_ticks, g_sounds, g_stills);

    unsigned rows = (g_stills + Columns - 1) / Columns;
    uint8_t *sheet = calloc((size_t)Columns * 320 * rows * 200, 1);
    for (unsigned s = 0; s < g_stills; s++) {
        unsigned ox = (s % Columns) * 320, oy = (s / Columns) * 200;
        for (unsigned y = 0; y < 200; y++) {
            memcpy(sheet + (size_t)(oy + y) * Columns * 320 + ox, g_still[s] + y * 320, 320);
        }
    }
    uint8_t palette[768];
    for (unsigned i = 0; i < 256; i++) { /* 3-3-2, as 6-bit DAC values */
        palette[i * 3] = (uint8_t)(((i >> 5) & 7) * 63 / 7);
        palette[i * 3 + 1] = (uint8_t)(((i >> 2) & 7) * 63 / 7);
        palette[i * 3 + 2] = (uint8_t)((i & 3) * 63 / 3);
    }
    return writePng(argv[2], sheet, palette, Columns * 320, rows * 200, 1) ? 0 : 1;
}

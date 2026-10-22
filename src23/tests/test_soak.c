/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_soak test_soak.c ../gameui.c ../alchemylist.c ../confirmprompt.c ../partyinfo.c ../mapview.c ../monsterpanel.c ../textpanel.c ../session.c ../thrown.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
 *       ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c ../lighting.c \
 *       ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../spellrecord.c ../spellcast.c ../consumable.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c \
 *       ../conversation.c ../dialog.c ../dialogservice.c ../shop.c ../relics.c ../inventory.c ../maptrigger.c ../ailment.c ../document.c ../viewrender.c ../minimap.c ../statuspanel.c \
 *       ../font.c ../uiregions.c ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../music.c ../paperdoll.c ../statsheet.c ../roster.c ../pictures.c ../palette.c ../cluepaged.c \
 *       ../cluebook.c ../titlemenu.c ../creationwizard.c ../creation.c ../charcreate.c ../textfield.c ../clueentries.c ../cluetransport.c ../cluemonster.c ../cluespell.c ../cluemap.c \
 *       ../worldmap_stdio.c ../chargen.c ../clueitem.c ../exedata.c && ./test_soak
 *
 * A soak test of the whole front end: a long run of pseudo-random key presses and mouse clicks (every UiKey the platform can send, with and without Ctrl, plus the animation tick and a drawing of the
 * screen now and then) through the title menu, the roster, character creation, the clue book, the dialogs, shops and conversations, the combat and the walking, for each game. It
 * checks that nothing crashes, that the screen buffer and the party stay sane, and that the front end never answers "quit" except for Escape / Ctrl+Q at the top. Needs WORLD.DAT and
 * the executable of yendor2/game and yendor3/game (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override; SOAK_KEYS sets the number of keys per run, default 20000).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameui.h"

static int g_failureCount = 0;
static int g_skipCount = 0;
static unsigned g_effects, g_tracks;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static uint8_t *readFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static const uint8_t *picture(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[318 * 198];
    (void)ctx;
    memset(pixels, (int)(0x20 + category * 7 + id % 5), sizeof(pixels));
    return pixels;
}

static void onEffect(void *ctx, unsigned id) {
    (void)ctx;
    (void)id;
    g_effects++;
}

static void onMusic(void *ctx, unsigned track) {
    (void)ctx;
    (void)track;
    g_tracks++;
}

static uint8_t g_image[SaveFileSizeMax];
static size_t g_imageSize;

static bool onStore(void *ctx, const uint8_t *image, size_t size) {
    (void)ctx;
    memcpy(g_image, image, size);
    g_imageSize = size;
    return true;
}

static size_t onFetch(void *ctx, uint8_t *image, size_t capacity) {
    (void)ctx;
    if (!g_imageSize || g_imageSize > capacity) {
        return 0;
    }
    memcpy(image, g_image, g_imageSize);
    return g_imageSize;
}

static bool onCinematic(void *ctx, const char *kind) {
    (void)ctx;
    (void)kind;
    return true;
}

static unsigned long long g_state = 12345;

static unsigned next(unsigned n) { /* a small LCG: the same keys on every platform */
    g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)((g_state >> 33) % n);
}

static UiKey randomKey(void) {
    static const UiKey special[] = {UiKeyEscape, UiKeyEnter, UiKeyBackspace, UiKeyUp, UiKeyDown, UiKeyLeft, UiKeyRight, UiKeyF1, UiKeyF2, UiKeyF3, UiKeyF4, UiKeyF5, UiKeyF6, UiKeyF7, UiKeyF8,
                                    UiKeyF9, (UiKey)9, (UiKey)' ', (UiKey)'+', (UiKey)'-'};
    unsigned roll = next(10);
    if (roll < 3) {
        return special[next(sizeof(special) / sizeof(special[0]))];
    }
    if (roll < 5) {
        return (UiKey)('0' + next(10));
    }
    return (UiKey)('a' + next(26));
}

static void soakGame(GameKind game, const char *envName, const char *defaultDir, const char *exeName, const char *label, bool title, unsigned long long seed) {
    g_state = seed;
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size, exeSize;
    uint8_t *world = readFile(path, &size);
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, exeName);
    uint8_t *exeBytes = readFile(path, &exeSize);
    ExeData exe;
    if (!world || !exeBytes || !exeDataOpen(&exe, game, exeBytes, exeSize)) {
        printf("SKIP %s soak (game files not found)\n", label);
        g_skipCount++;
        free(world);
        free(exeBytes);
        return;
    }
    unsigned keys = getenv("SOAK_KEYS") ? (unsigned)atoi(getenv("SOAK_KEYS")) : 20000;
    GameSession *s = sessionNew(game, world, size);
    static GameUi ui;
    GameUiHost host = {NULL, onEffect, onMusic, onCinematic, onStore, onFetch};
    if (!s || !gameUiInit(&ui, s, world, size, picture, NULL, &host)) {
        check("the session and the UI start", false);
        return;
    }
    gameUiSetExe(&ui, &exe);
    ui.clueRegistered = (game == GameYendor3) || (next(2) == 0);
    if (title) {
        gameUiShowTitle(&ui);
    }
    bool sane = true, quitEarly = false, partyOk = true;
    unsigned quits = 0, overlays[16] = {0};
    for (unsigned i = 0; i < keys; i++) {
        UiKey key = randomKey();
        bool ctrl = next(40) == 0;
        if (ui.overlay == UiOverlayNone && next(5000) == 0) { /* now and then a game minute passes, like the platform's timer */
            sessionAdvanceTime(s, 1);
        }
        bool running;
        UiOverlay was = ui.overlay;
        if (next(4) == 0) { /* a mouse click anywhere on the 320 x 200 screen */
            key = UiKeyNone;
            running = gameUiClick(&ui, (int)next(320), (int)next(200), next(6) == 0);
        } else {
            running = gameUiKey(&ui, key, ctrl);
        }
        if (!running) {
            quits++;
            bool allowed = key == UiKeyEscape || (key == 'q' && ctrl) || key == UiKeyNone || (was == UiOverlayDialog && (key == 'y' || key == UiKeyEnter)); /* the dialog's DOS option, once its question is answered (a click can do it too) */
            quitEarly = quitEarly || !allowed;
            if (title) {
                gameUiShowTitle(&ui); /* start over: the platform would have ended */
            }
        }
        if (ui.overlay < 16) {
            overlays[ui.overlay]++;
        }
        if (i % 7 == 0) {
            gameUiAnimate(&ui);
        }
        if (i % 41 == 0) {
            gameUiDraw(&ui);
        }
        for (unsigned slot = 0; slot < 4; slot++) {
            unsigned id = saveHeaderGetU16(&s->save, SaveHeaderPartySlots + 2 * slot);
            partyOk = partyOk && id <= SavePartyRecordCount;
        }
        sane = sane && s->x >= 0 && s->y >= 0 && s->facing != 0 && ui.rosterCursor < 9 && ui.wizardSlot < SavePartyRecordCount && ui.clueList.selected <= ui.clueList.count + 1;
        if (!sane || !partyOk) {
            printf("  stopped at key %u (%d) overlay %u\n", i, (int)key, (unsigned)ui.overlay);
            break;
        }
    }
    gameUiDraw(&ui);
    char text[160];
    snprintf(text, sizeof(text), "%s (%s start, seed %llu): %u random keys leave the front end sane (positions, party slots, cursors)", label, title ? "title" : "game", seed, keys);
    check(text, sane && partyOk);
    check("it only ever quits on Escape, Ctrl+Q or the pause dialog's DOS question answered yes", !quitEarly);
    unsigned reached = 0;
    for (unsigned o = 0; o < 16; o++) {
        reached += overlays[o] > 0;
    }
    snprintf(text, sizeof(text), "%s: the keys visited %u of the overlays (quit %u times, %u effects, %u tracks)", label, reached, quits, g_effects, g_tracks);
    check(text, reached >= (title ? 6u : 4u));
    sessionFree(s);
    free(world);
    free(exeBytes);
}

int main(void) {
    for (unsigned run = 0; run < 4; run++) { /* the title menu first, then straight into the game; two seeds each */
        soakGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2", run % 2 == 0, 12345 + run / 2);
        soakGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3", run % 2 == 0, 777 + run / 2);
    }
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

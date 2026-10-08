/*
 * An interactive walk through a real map: the game core (session.h) driven by a keyboard in an SDL2 window. Everything that is game logic -- movement and
 * passability, the fog-of-war reveal, the monsters and the combat, resting, locks and chests -- is in the session; this file only reads keys, draws the screen
 * (the first-person view with the day/night lighting, the minimap, the four party panels, the local area map, the pause dialog, the hero sheets and paper dolls)
 * and plays sound (the area music through the OPL2 synthesizer, a couple of effects). It is the smallest "engine" built from the modules.
 *
 * Build and run (from src23/tools; SDL2 from C:\sdk\SDL2-2.32.10, SDL2.dll next to the exe or on PATH):
 *   gcc -Wall -Wextra -std=c99 -I .. -I /c/sdk/SDL2-2.32.10/include -o explore_sdl explore_sdl.c ../gameui.c ../session.c ../windowbake.c ../interact.c ../lockcatalog.c \
 *       ../worldobjects.c ../monsterpool.c ../monster.c ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../cmfplayer.c ../opl.c ../cmf.c \
 *       ../audio.c ../voc.c ../music.c ../spellrecord.c ../paperdoll.c ../statsheet.c ../roster.c ../viewrender.c ../minimap.c ../statuspanel.c ../font.c \
 *       ../uiregions.c ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c \
 *       ../introstory.c ../intro2.c ../palettefade.c ../exedata.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c ../conversation.c ../dialog.c ../dialogservice.c ../shop.c ../relics.c ../inventory.c ../maptrigger.c ../ailment.c ../document.c ../spellcast.c ../consumable.c \
 *       ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../palette.c \
 *       -L /c/sdk/SDL2-2.32.10/lib -lmingw32 -lSDL2main -lSDL2 -lm
 *   ./explore_sdl <2|3> <game dir> [scale]
 *
 * Keys: Up / Down walk, Left / Right turn, Ctrl+Left / Ctrl+Right strafe (the original's scan codes, through mainCommandForKey), A attacks while a combat is on (C casts the first attack spell the member can, and outside a combat the next support / exploration spell any member can: the party starts with none of the ore the spells cost, so loot a chest first),
 * R rest (eight hours; monsters can interrupt it), K unlock the door ahead with a skeleton key, S use the object ahead (loot the chest, take a road or portal, play the Chapter 2 ending at the Quartz Chamber), M the local area map, D the pause
 * dialog, P the paper dolls, F5 saves / F9 loads explore.sav (the original save-file layout), F1-F4 the hero's detail sheet, + / - move the clock by 30 minutes (watch the lighting), Escape closes an overlay or quits.
 *
 * EXPLORE_INTRO=opening|story|ending plays a cinematic of the game first (opening = the opening and the story; Chapter 3 has only its opening; EXPLORE_INTRO_MS sets
 * the milliseconds per tick, 25 by default; EXPLORE_INTRO_SOUND=1 plays the voices instead of drawing the text); Escape ends it.
 *
 * Headless check: with EXPLORE_KEYS set (F B L R forward / back / turn left / turn right, Q E strafe, M map, T rest, D dialog, K, S, A, P, 1 2, + -, ESC as '!') the
 * keys are played at start and the final screen is written to the PNG named by EXPLORE_SHOT, then the program exits (SDL_VIDEODRIVER=dummy needs no display;
 * SDL_AUDIODRIVER=dummy or EXPLORE_NOSOUND=1 silences it). EXPLORE_CLOCK_MS=<n> lets one game minute pass every n ms (sessionAdvanceTime), EXPLORE_RANDOM=<n> plays n random keys, EXPLORE_START=x,y,N|E|S|W starts elsewhere, EXPLORE_DEBUG=1
 * lists the monster markers in view.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "audio.h"
#include "cmfplayer.h"
#include "exedata.h"
#include "gameui.h"
#include "introstory.h"
#include "pictures_stdio.h"
#include "pngwrite.h"
#include "voc.h"

static GameUi g_ui;
static GameSession *g_session;

/* Sound: the area music through the OPL2 synthesizer (opl.c), effects as VOC samples, both mixed in SDL's audio callback. */
static uint8_t g_worldDat[5000000];
static size_t g_worldSize;
static CmfPlayer g_music;
static bool g_musicOn;
static unsigned g_musicTrack;
static SDL_AudioDeviceID g_audio;
static int16_t g_effect[44100 * 3];
static unsigned g_effectLength, g_effectPos;

static void audioCallback(void *userdata, Uint8 *stream, int bytes) {
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    unsigned count = (unsigned)bytes / 2;
    memset(out, 0, (size_t)bytes);
    if (g_musicOn) {
        unsigned got = cmfPlayerRender(&g_music, out, count);
        for (unsigned i = 0; i < got; i++) {
            out[i] = (int16_t)(out[i] / 3); /* the tracks are loud enough at a third */
        }
        if (got < count) { /* the track ended: loop it */
            uint32_t offset, length;
            if (audioMusicTrack(g_session->game, g_musicTrack, &offset, &length) && offset + length <= g_worldSize) {
                cmfPlayerStart(&g_music, g_worldDat + offset, length, 44100);
            } else {
                g_musicOn = false;
            }
        }
    }
    for (unsigned i = 0; i < count && g_effectPos < g_effectLength; i++, g_effectPos++) {
        int v = out[i] + g_effect[g_effectPos];
        out[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
    }
}

static void playMusic(void *ctx, unsigned track) {
    (void)ctx;
    uint32_t offset, length;
    if (!g_audio) {
        return;
    }
    SDL_LockAudioDevice(g_audio);
    g_musicTrack = track;
    g_musicOn = track && audioMusicTrack(g_session->game, track, &offset, &length) && offset + length <= g_worldSize && cmfPlayerStart(&g_music, g_worldDat + offset, length, 44100);
    SDL_UnlockAudioDevice(g_audio);
}

/* Sound effect id (WORLD.DAT's VOC list) resampled to 44.1 kHz. */
static void playEffect(void *ctx, unsigned id) {
    (void)ctx;
    uint32_t offset, length;
    VocSound voc;
    if (!g_audio || !audioEffect(g_session->game, id, &offset, &length) || offset + length > g_worldSize || !vocOpen(&voc, g_worldDat + offset, length)) {
        return;
    }
    SDL_LockAudioDevice(g_audio);
    unsigned n = (unsigned)((double)voc.length * 44100.0 / voc.sampleRate);
    n = n > sizeof(g_effect) / sizeof(g_effect[0]) ? (unsigned)(sizeof(g_effect) / sizeof(g_effect[0])) : n;
    for (unsigned i = 0; i < n; i++) {
        g_effect[i] = (int16_t)(((int)voc.samples[(size_t)((double)i * voc.sampleRate / 44100.0)] - 128) * 100);
    }
    g_effectLength = n;
    g_effectPos = 0;
    SDL_UnlockAudioDevice(g_audio);
}

/* SDL keys -> the UI's keys; letters and digits are their lower-case ASCII. */
static UiKey uiKeyFor(SDL_Keycode key) {
    switch (key) {
    case SDLK_UP: return UiKeyUp;
    case SDLK_DOWN: return UiKeyDown;
    case SDLK_LEFT: return UiKeyLeft;
    case SDLK_RIGHT: return UiKeyRight;
    case SDLK_ESCAPE: return UiKeyEscape;
    case SDLK_PLUS: case SDLK_KP_PLUS: case SDLK_EQUALS: return (UiKey)'+';
    case SDLK_MINUS: case SDLK_KP_MINUS: return (UiKey)'-';
    default:
        if (key >= SDLK_F1 && key <= SDLK_F9) {
            return (UiKey)(UiKeyF1 + (key - SDLK_F1));
        }
        if ((key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_0 && key <= SDLK_9)) {
            return (UiKey)key;
        }
        return UiKeyNone;
    }
}

static SDL_Keycode scriptKey(char c, Uint16 *mod) {
    *mod = 0;
    switch (c) {
    case 'F': return SDLK_UP;
    case 'B': return SDLK_DOWN;
    case 'L': return SDLK_LEFT;
    case 'R': return SDLK_RIGHT;
    case 'Q': *mod = KMOD_CTRL; return SDLK_LEFT;
    case 'E': *mod = KMOD_CTRL; return SDLK_RIGHT;
    case 'M': return SDLK_m;
    case 'T': return SDLK_r;
    case 'K': return SDLK_k;
    case 'S': return SDLK_s;
    case 'A': return SDLK_a;
    case 'C': return SDLK_c;
    case 'D': return SDLK_d;
    case 'P': return SDLK_p;
    case '1': return SDLK_F1;
    case '2': return SDLK_F2;
    case '+': return SDLK_PLUS;
    case '-': return SDLK_MINUS;
    case '!': return SDLK_ESCAPE;
    case '#': return SDLK_1; /* plain digit keys for the talk and shop overlays */
    case '$': return SDLK_2;
    case '%': return SDLK_3;
    case '&': return SDLK_4;
    case 'W': return SDLK_F5; /* save */
    case 'O': return SDLK_F9; /* load */
    default: return SDLK_UNKNOWN;
    }
}

/* The save image goes to explore.sav in the current directory. */
static bool storeSave(void *ctx, const uint8_t *image, size_t size) {
    (void)ctx;
    FILE *f = fopen("explore.sav", "wb");
    if (!f) {
        return false;
    }
    bool ok = fwrite(image, 1, size, f) == size;
    fclose(f);
    return ok;
}

static size_t fetchSave(void *ctx, uint8_t *image, size_t capacity) {
    (void)ctx;
    FILE *f = fopen("explore.sav", "rb");
    if (!f) {
        return 0;
    }
    size_t bytes = fread(image, 1, capacity, f);
    fclose(f);
    return bytes;
}

/* ---- the cinematics (introstory.h): EXPLORE_INTRO=opening|story|ending plays one in the window before the game; Escape ends it --------------------------------- */
static SDL_Renderer *g_introRenderer;
static SDL_Texture *g_introTexture;
static const char *g_dir;
static PictureFile *g_pictures;
static bool g_introEscape;
static unsigned g_introDelayMs = 25;

static void introPump(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            g_introEscape = true;
        }
    }
}

static void introPresent(void *ctx, const uint8_t *screen, const uint8_t *dac) {
    (void)ctx;
    static uint32_t pixels[ViewScreenWidth * ViewScreenHeight];
    uint32_t colours[256];
    for (unsigned i = 0; i < 256; i++) {
        unsigned r = dac[i * 3] & 63, g = dac[i * 3 + 1] & 63, b = dac[i * 3 + 2] & 63;
        colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
    }
    for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
        pixels[i] = colours[screen[i]];
    }
    SDL_UpdateTexture(g_introTexture, NULL, pixels, ViewScreenWidth * 4);
    SDL_RenderClear(g_introRenderer);
    SDL_RenderCopy(g_introRenderer, g_introTexture, NULL, NULL);
    SDL_RenderPresent(g_introRenderer);
}

static void introTick(void *ctx) {
    (void)ctx;
    introPump();
    if (g_introDelayMs) {
        SDL_Delay(g_introDelayMs);
    }
}

static bool introEscapeFn(void *ctx) {
    (void)ctx;
    introPump();
    return g_introEscape;
}

static bool introSoundBusy(void *ctx) {
    (void)ctx;
    return g_effectPos < g_effectLength;
}

static void introWaitSound(void *ctx) {
    while (introSoundBusy(ctx) && !g_introEscape) {
        SDL_Delay(10);
        introPump();
    }
}

static bool playIntro(void *ctx, const char *kind) {
    (void)ctx;
    GameKind game = g_session->game;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_dir, game == GameYendor3 ? "REGISTER.EXE" : "SW.EXE");
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        return false;
    }
    static uint8_t exeBytes[400000];
    size_t exeSize = fread(exeBytes, 1, sizeof(exeBytes), f);
    fclose(f);
    ExeData exe;
    if (!exeDataOpen(&exe, game, exeBytes, exeSize)) {
        fprintf(stderr, "%s is not the game executable\n", path);
        return false;
    }
    IntroAssets assets = {g_worldDat, g_worldSize, &exe, pictureFileGet, g_pictures, game};
    static IntroStory story;
    if (getenv("EXPLORE_INTRO_MS")) {
        g_introDelayMs = (unsigned)atoi(getenv("EXPLORE_INTRO_MS"));
    }
    g_introEscape = false;
    IntroHost host = {NULL, introPresent, playEffect, playMusic, introTick, introEscapeFn, NULL, getenv("EXPLORE_INTRO_SOUND") != NULL, introWaitSound, introSoundBusy};
    bool ok;
    if (game == GameYendor3) {
        ok = introOpening3Start(&story, &assets) && introOpening3Play(&story, &assets, &host);
    } else if (strcmp(kind, "ending") == 0) {
        ok = introEndingStart(&story, &assets) && introEndingPlay(&story, &assets, &host);
    } else if (strcmp(kind, "story") == 0) {
        ok = introStoryStart(&story, &assets) && introStoryPlay(&story, &assets, &host);
    } else {
        ok = introOpeningStart(&story, &assets) && introPlayAll(&story, &assets, &host);
    }
    playMusic(NULL, 0);
    return ok || g_introEscape;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <2|3> <game dir> [scale]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    const char *dir = argv[2];
    g_dir = dir;
    int scale = argc > 3 ? atoi(argv[3]) : 3;
    if (scale < 1) {
        scale = 3;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    FILE *f = fopen(path, "rb");
    g_worldSize = f ? fread(g_worldDat, 1, sizeof(g_worldDat), f) : 0;
    if (f) {
        fclose(f);
    }
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir);
    g_pictures = pictureFileOpen(path, game);
    if (!g_pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    g_session = sessionNew(game, g_worldDat, g_worldSize);
    if (!g_session) {
        fprintf(stderr, "cannot start a session (is %s/WORLD.DAT there?)\n", dir);
        return 1;
    }
    if (getenv("EXPLORE_START")) { /* x,y,N|E|S|W: start somewhere else (testing) */
        int sx, sy;
        char dirChar = 'N';
        if (sscanf(getenv("EXPLORE_START"), "%d,%d,%c", &sx, &sy, &dirChar) >= 2) {
            g_session->x = sx;
            g_session->y = sy;
            g_session->facing = dirChar == 'S' ? SaveFacingSouth : dirChar == 'E' ? SaveFacingEast : dirChar == 'W' ? SaveFacingWest : SaveFacingNorth;
            sessionRebuild(g_session);
        }
    }
    GameUiHost uiHost = {NULL, playEffect, playMusic, playIntro, storeSave, fetchSave};
    if (!gameUiInit(&g_ui, g_session, g_worldDat, g_worldSize, pictureFileGet, g_pictures, &uiHost)) {
        fprintf(stderr, "the world data lacks the view tables\n");
        return 1;
    }
    g_ui.debugMarkers = getenv("EXPLORE_DEBUG") != NULL;
    const uint8_t *palette = gameUiPalette(&g_ui);
    if (!palette) {
        fprintf(stderr, "the world data lacks the master palette\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Yendorian Tales", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ViewScreenWidth * scale, ViewScreenHeight * scale, 0);
    SDL_Renderer *sdl = window ? SDL_CreateRenderer(window, -1, 0) : NULL;
    SDL_Texture *texture = sdl ? SDL_CreateTexture(sdl, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, ViewScreenWidth, ViewScreenHeight) : NULL;
    if (!texture) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    g_introRenderer = sdl;
    g_introTexture = texture;
    uint32_t colours[256];
    for (unsigned i = 0; i < 256; i++) {
        unsigned r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63, b = palette[i * 3 + 2] & 63;
        colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
    }
    if (!getenv("EXPLORE_NOSOUND")) {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 1;
        want.samples = 2048;
        want.callback = audioCallback;
        g_audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (g_audio) {
            gameUiAmbientMusic(&g_ui);
            SDL_PauseAudioDevice(g_audio, 0);
        }
    }

    if (getenv("EXPLORE_INTRO") && !playIntro(NULL, getenv("EXPLORE_INTRO"))) {
        return 1;
    }
    bool running = true, dirty = true;
    const char *script = getenv("EXPLORE_KEYS");
    static char randomScript[20001];
    if (getenv("EXPLORE_RANDOM")) { /* EXPLORE_RANDOM=<n>: n random keys (a soak test; EXPLORE_SHOT still writes the last screen) */
        unsigned n = (unsigned)atoi(getenv("EXPLORE_RANDOM"));
        const char alphabet[] = "FFFFFFBLRLRQEAAATCSS";
        for (unsigned i = 0; i < n && i < sizeof(randomScript) - 1; i++) {
            randomScript[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
        }
        script = randomScript;
    }
    const char *shotPath = getenv("EXPLORE_SHOT");
    Uint32 lastMinute = 0;
    unsigned clockMs = getenv("EXPLORE_CLOCK_MS") ? (unsigned)atoi(getenv("EXPLORE_CLOCK_MS")) : 0; /* real milliseconds per game minute; 0 = the clock only moves with + / - and resting */
    while (running) {
        if (script && *script) { /* feed the next scripted key as a key press */
            SDL_Event press;
            memset(&press, 0, sizeof(press));
            Uint16 mod;
            SDL_Keycode key = scriptKey(*script++, &mod);
            if (key != SDLK_UNKNOWN) {
                press.type = SDL_KEYDOWN;
                press.key.keysym.sym = key;
                press.key.keysym.mod = mod;
                SDL_PushEvent(&press);
            }
        } else if (script && shotPath) {
            gameUiDraw(&g_ui);
            writePng(shotPath, g_ui.screen, palette, ViewScreenWidth, ViewScreenHeight, 2);
            if (g_session->log[0]) {
                printf("log: %s\n", g_session->log);
            }
            printf("(%d, %d) facing %u clock %u -> %s\n", g_session->x, g_session->y, (unsigned)g_session->facing, g_session->clock.minutes, shotPath);
            break;
        }
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, script && *script ? 0 : 100)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                UiKey key = uiKeyFor(event.key.keysym.sym);
                if (key != UiKeyNone) {
                    running = gameUiKey(&g_ui, key, (event.key.keysym.mod & KMOD_CTRL) != 0) && running;
                }
                dirty = true;
            } else if (event.type == SDL_WINDOWEVENT) {
                dirty = true;
            }
        }
        if (clockMs && !g_session->combat.active && SDL_GetTicks() - lastMinute >= clockMs) {
            lastMinute = SDL_GetTicks();
            sessionAdvanceTime(g_session, 1);
            dirty = true;
        }
        if (dirty) {
            gameUiDraw(&g_ui);
            static uint32_t pixels[ViewScreenWidth * ViewScreenHeight];
            for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
                pixels[i] = colours[g_ui.screen[i]];
            }
            SDL_UpdateTexture(texture, NULL, pixels, ViewScreenWidth * 4);
            SDL_RenderClear(sdl);
            SDL_RenderCopy(sdl, texture, NULL, NULL);
            SDL_RenderPresent(sdl);
            char title[256];
            gameUiStatus(&g_ui, title, sizeof(title));
            SDL_SetWindowTitle(window, title);
            dirty = false;
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    sessionFree(g_session);
    pictureFileClose(g_pictures);
    return 0;
}

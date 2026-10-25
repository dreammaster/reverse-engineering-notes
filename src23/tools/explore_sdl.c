/*
 * An interactive walk through a real map: the game core (session.h) driven by a keyboard in an SDL2 window. Everything that is game logic -- movement and
 * passability, the fog-of-war reveal, the monsters and the combat, resting, locks and chests -- is in the session; this file only reads keys, draws the screen
 * (the first-person view with the day/night lighting, the minimap, the four party panels, the local area map, the pause dialog, the hero sheets and paper dolls)
 * and plays sound (the area music through the OPL2 synthesizer, a couple of effects). It is the smallest "engine" built from the modules.
 *
 * Build and run (from src23/tools; SDL2 from C:\sdk\SDL2-2.32.10, SDL2.dll next to the exe or on PATH):
 *   gcc -Wall -Wextra -std=c99 -I .. -I /c/sdk/SDL2-2.32.10/include -o explore_sdl explore_sdl.c ../gameui.c ../alchemylist.c ../confirmprompt.c ../partyinfo.c ../mapview.c ../monsterpanel.c ../textpanel.c ../session.c ../thrown.c ../windowbake.c ../interact.c ../lockcatalog.c \
 *       ../worldobjects.c ../monsterpool.c ../monster.c ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../cmfplayer.c ../opl.c ../cmf.c \
 *       ../audio.c ../voc.c ../music.c ../spellrecord.c ../paperdoll.c ../statsheet.c ../roster.c ../viewrender.c ../minimap.c ../statuspanel.c ../font.c \
 *       ../uiregions.c ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c \
 *       ../introstory.c ../intro2.c ../palettefade.c ../exedata.c ../cluepaged.c ../cluebook.c ../clueentries.c ../clueitem.c ../cluetransport.c ../cluemonster.c ../cluemap.c ../cluespell.c ../pcx.c ../chargen.c ../creationwizard.c ../creation.c ../charcreate.c ../textfield.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c ../conversation.c ../dialog.c ../dialogservice.c ../shop.c ../relics.c ../inventory.c ../container.c ../repair.c ../mount.c ../maptrigger.c ../ailment.c ../document.c ../titlemenu.c ../spellcast.c ../consumable.c \
 *       ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../palette.c \
 *       -L /c/sdk/SDL2-2.32.10/lib -lmingw32 -lSDL2main -lSDL2 -lm
 *   ./explore_sdl [options] <game dir>      (--help lists them: --game 2|3, --scale N, --no-splash, --no-title, --shareware, --no-sound, --clock-ms N; the chapter is found from the folder)
 *   ./explore_sdl <2|3> <game dir> [scale]  (the older form: starts in the game, not on the title menu)
 *
 * Keys: Up / Down walk, Left / Right turn, Ctrl+Left / Ctrl+Right strafe (the original's scan codes, through mainCommandForKey), A attacks while a combat is on (C casts the first attack spell the member can, and outside a combat the next support / exploration spell any member can: the party starts with none of the ore the spells cost, so loot a chest first),
 * R rest (eight hours; monsters can interrupt it), K unlock the door ahead with a skeleton key, Space (or the right button on the view) use the object ahead (loot the chest, take a road or portal, play the Chapter 2 ending at the Quartz Chamber), S shoot: the heroes fire their weapons down the corridor (four rocks or arrows fly away), M the local area map, D the pause
 * dialog, P the paper dolls, F7 saves / F9 loads explore.sav (the original save-file layout; F5 is the top-right panel), F1-F4 the hero's detail sheet, + / - move the clock by 30 minutes (watch the lighting), Escape closes an overlay or quits.
 *
 * EXPLORE_INTRO=opening|story|ending|credits plays a cinematic of the game first (opening = the opening and the story; Chapter 3 has only its opening; EXPLORE_INTRO_MS sets
 * the milliseconds per tick, 25 by default; EXPLORE_INTRO_SOUND=1 plays the voices instead of drawing the text); Escape ends it.
 *
 * Headless check: with EXPLORE_KEYS set (F B L R forward / back / turn left / turn right, Q E strafe, M map, T rest, D dialog, K, S, A, P, 1 2, + -, ESC as '!') the
 * keys are played at start and the final screen is written to the PNG named by EXPLORE_SHOT, then the program exits (SDL_VIDEODRIVER=dummy needs no display;
 * SDL_AUDIODRIVER=dummy or EXPLORE_NOSOUND=1 silences it). EXPLORE_TITLE=1 starts on the title menu, EXPLORE_CLOCK_MS=<n> lets one game minute pass every n ms (sessionAdvanceTime), EXPLORE_RANDOM=<n> plays n random keys, EXPLORE_START=x,y,N|E|S|W starts elsewhere, EXPLORE_DEBUG=1
 * lists the monster markers in view, EXPLORE_TICKS=<n> lets n animation ticks pass before the picture (a shot in flight, a pressed button).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "audio.h"
#include "explore.h"
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
    case SDLK_PAGEUP: return UiKeyPageUp;
    case SDLK_PAGEDOWN: return UiKeyPageDown;
    case SDLK_ESCAPE: return UiKeyEscape;
    case SDLK_TAB: return (UiKey)9;
    case SDLK_BACKSPACE: return UiKeyBackspace;
    case SDLK_SPACE: return (UiKey)' ';
    case SDLK_RETURN: case SDLK_KP_ENTER: return UiKeyEnter;
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
    case '3': return SDLK_F3;
    case '4': return SDLK_F4;
    case '5': return SDLK_F5;
    case '+': return SDLK_PLUS;
    case '-': return SDLK_MINUS;
    case '!': return SDLK_ESCAPE;
    case '#': return SDLK_1; /* plain digit keys for the talk and shop overlays */
    case '$': return SDLK_2;
    case '%': return SDLK_3;
    case '&': return SDLK_4;
    case 'Y': return SDLK_F8; /* the clue book; X = F6 (walk through), Z = Tab (help), I / J = I / Q (turn pages) */
    case 'X': return SDLK_F6;
    case 'Z': return SDLK_TAB;
    case 'N': return SDLK_RETURN;
    case 'I': return SDLK_i;
    case 'J': return SDLK_q;
    case 'W': return SDLK_F7; /* save */
    case 'O': return SDLK_F9; /* load */
    case '_': return SDLK_SPACE;
    case '<': return SDLK_BACKSPACE;
    case '^': return SDLK_5;
    case '*': return SDLK_6;
    case '(': return SDLK_7;
    case ')': return SDLK_8;
    case '=': return SDLK_9;
    default:
        return c >= 'a' && c <= 'z' ? SDLK_a + (c - 'a') : SDLK_UNKNOWN; /* lower case: the plain letter keys (character creation, names) */
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

/* The six save slots of the pause dialog are the original's own files, SAVGAME1 .. SAVGAME6 in the game folder (a save is a copy of CURGAME, savegame.h), so the saves of the DOS game load here
 * and the other way round. */
static const char *g_dir;

static FILE *slotFile(unsigned slot, const char *mode) {
    char path[600];
    snprintf(path, sizeof(path), "%s/SAVGAME%u", g_dir, slot + 1);
    return fopen(path, mode);
}

static bool slotName(void *ctx, unsigned slot, char *name, size_t capacity) {
    (void)ctx;
    FILE *f = slotFile(slot, "rb");
    if (!f) {
        return false;
    }
    char header[SaveNameBufferSize] = {0};
    size_t bytes = fread(header, 1, SaveNameMaxLength, f);
    fclose(f);
    if (bytes == 0) {
        return false;
    }
    snprintf(name, capacity, "%s", header);
    return true;
}

static bool slotWrite(void *ctx, unsigned slot, const uint8_t *image, size_t size) {
    (void)ctx;
    FILE *f = slotFile(slot, "wb");
    if (!f) {
        return false;
    }
    bool ok = fwrite(image, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

static size_t slotRead(void *ctx, unsigned slot, uint8_t *image, size_t capacity) {
    (void)ctx;
    FILE *f = slotFile(slot, "rb");
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
static PictureFile *g_pictures;
static bool g_introEscape, g_quit;
static unsigned g_introDelayMs = 25;

static void introPump(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            g_introEscape = true;
            g_quit = g_quit || event.type == SDL_QUIT;
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

/* Chapter 3's 640 x 480 boot logo: its own texture, letter-boxed into the window. */
static void introHires(void *ctx, const uint8_t *picture, unsigned width, unsigned height, const uint8_t *palette) {
    (void)ctx;
    static uint32_t pixels[640 * 480];
    uint32_t colours[256];
    if (width > 640 || height > 480) {
        return;
    }
    for (unsigned i = 0; i < 256; i++) {
        unsigned r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63, b = palette[i * 3 + 2] & 63;
        colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
    }
    for (unsigned i = 0; i < width * height; i++) {
        pixels[i] = colours[picture[i]];
    }
    SDL_Texture *texture = SDL_CreateTexture(g_introRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, (int)width, (int)height);
    if (!texture) {
        return;
    }
    SDL_UpdateTexture(texture, NULL, pixels, (int)width * 4);
    int outW, outH;
    SDL_GetRendererOutputSize(g_introRenderer, &outW, &outH);
    int fitW = outW, fitH = outW * (int)height / (int)width;
    if (fitH > outH) {
        fitH = outH;
        fitW = outH * (int)width / (int)height;
    }
    SDL_Rect where = {(outW - fitW) / 2, (outH - fitH) / 2, fitW, fitH};
    SDL_RenderClear(g_introRenderer);
    SDL_RenderCopy(g_introRenderer, texture, NULL, &where);
    SDL_RenderPresent(g_introRenderer);
    SDL_DestroyTexture(texture);
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

/* The settings: the EXPLORE_* environment variables (the scripted and headless runs) and the command line flags, which set the same names. */
static const char *g_override[24][2];
static unsigned g_overrideCount;

static void setOpt(const char *name, const char *value) {
    if (g_overrideCount < sizeof(g_override) / sizeof(g_override[0])) {
        g_override[g_overrideCount][0] = name;
        g_override[g_overrideCount][1] = value;
        g_overrideCount++;
    }
}

static const char *opt(const char *name) {
    for (unsigned i = 0; i < g_overrideCount; i++) {
        if (strcmp(g_override[i][0], name) == 0) {
            return g_override[i][1];
        }
    }
    return getenv(name);
}

static ExeData g_exe;

/* The executable (SW.EXE / REGISTER.EXE): its data segment holds the cinematic and clue book texts. */
static bool loadExe(GameKind game) {
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
    if (!exeDataOpen(&g_exe, game, exeBytes, exeSize)) {
        fprintf(stderr, "%s is not the game executable\n", path);
        return false;
    }
    return true;
}

static bool playIntro(void *ctx, const char *kind) {
    (void)ctx;
    GameKind game = g_session->game;
    ExeData exe = g_exe;
    if (!exe.data) {
        return false;
    }
    IntroAssets assets = {g_worldDat, g_worldSize, &exe, pictureFileGet, g_pictures, game};
    static IntroStory story;
    if (opt("EXPLORE_INTRO_MS")) {
        g_introDelayMs = (unsigned)atoi(opt("EXPLORE_INTRO_MS"));
    }
    g_introEscape = false;
    IntroHost host = {NULL, introPresent, playEffect, playMusic, introTick, introEscapeFn, NULL, opt("EXPLORE_INTRO_SOUND") != NULL, introWaitSound, introSoundBusy, introHires};
    bool ok;
    if (strcmp(kind, "splash") == 0) {
        ok = introSplashStart(&story, &assets) && introSplashPlay(&story, &assets, &host);
    } else if (strcmp(kind, "credits") == 0) {
        ok = introCreditsStart(&story, &assets) && introCreditsPlay(&story, &assets, &host);
    } else if (game == GameYendor3) {
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

static void usage(const char *program) {
    fprintf(stderr,
            "usage: %s [options] <game dir>\n"
            "  <game dir>      the folder with WORLD.DAT, PICTURES.VGA and SW.EXE (Chapter 2) or REGISTER.EXE (Chapter 3); the chapter is found from the files\n"
            "  --game 2|3      the chapter, when the folder has both or neither executable\n"
            "  --scale N       the window is N times 320 x 200 (default 3)\n"
            "  --no-splash     skip the boot title and the introduction        --no-title   start in the game: no start-up sequence at all\n"
            "  --shareware     the unregistered program (the demo boundary, the clue book's locked entries)\n"
            "  --no-sound      silence\n"
            "  --clock-ms N    one game minute passes every N real milliseconds (5000, the default, is the original's pace -- a day is two hours; 0 stops the clock: only + / - and resting move it)\n"
            "Keys: arrows walk and turn, F1-F4 the heroes, F5 / F9 save / load, F8 the clue book, M the map, P the paper dolls, D the pause dialog, Esc backs out; the mouse works too; on the title menu C creates a\n"
            "character, A assembles the party, E enters the game, I plays the introduction, R the credits. The older form  %s <2|3> <game dir> [scale]  still works.\n",
            program, program);
}

static bool fileExists(const char *dir, const char *name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (f) {
        fclose(f);
    }
    return f != NULL;
}

int main(int argc, char **argv) {
    const char *dir = NULL;
    int chapter = 0, scale = 3;
    if (argc >= 3 && (strcmp(argv[1], "2") == 0 || strcmp(argv[1], "3") == 0)) { /* the older form: <2|3> <game dir> [scale] */
        chapter = atoi(argv[1]);
        dir = argv[2];
        scale = argc > 3 ? atoi(argv[3]) : 3;
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
                chapter = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
                scale = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--clock-ms") == 0 && i + 1 < argc) {
                setOpt("EXPLORE_CLOCK_MS", argv[++i]);
            } else if (strcmp(argv[i], "--no-splash") == 0) {
                setOpt("EXPLORE_NOSPLASHFLAG", "1");
            } else if (strcmp(argv[i], "--no-title") == 0) {
                setOpt("EXPLORE_NOTITLEFLAG", "1");
            } else if (strcmp(argv[i], "--shareware") == 0) {
                setOpt("EXPLORE_SHAREWARE", "1");
            } else if (strcmp(argv[i], "--no-sound") == 0) {
                setOpt("EXPLORE_NOSOUND", "1");
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
                return 0;
            } else if (argv[i][0] == '-' && argv[i][1] == '-') {
                fprintf(stderr, "unknown option %s\n", argv[i]);
                usage(argv[0]);
                return 2;
            } else if (!dir) {
                dir = argv[i];
            } else {
                usage(argv[0]);
                return 2;
            }
        }
        if (!dir) {
            usage(argv[0]);
            return 2;
        }
        if (chapter == 0) { /* the chapter is whichever executable is in the folder */
            bool two = fileExists(dir, "SW.EXE"), three = fileExists(dir, "REGISTER.EXE");
            chapter = three && !two ? 3 : two && !three ? 2 : 0;
            if (!chapter) {
                fprintf(stderr, "cannot tell the chapter from %s (SW.EXE for Chapter 2, REGISTER.EXE for Chapter 3): use --game 2|3\n", dir);
                return 1;
            }
        }
        if (!opt("EXPLORE_KEYS") && !opt("EXPLORE_SHOT") && !opt("EXPLORE_RANDOM") && !opt("EXPLORE_INTRO")) { /* interactive: the original's flow */
            if (!opt("EXPLORE_NOTITLEFLAG")) {
                setOpt("EXPLORE_BOOT", "1");
                if (!opt("EXPLORE_NOSPLASHFLAG")) {
                    setOpt("EXPLORE_SPLASH", "1");
                }
            }
        }
    }
    GameKind game = chapter == 3 ? GameYendor3 : GameYendor2;
    g_dir = dir;
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
    if (opt("EXPLORE_MOUNTS")) { /* EXPLORE_MOUNTS=1 (testing): the first hero knows all four mounts, and the cells around the party are explored */
        partySetU16(sessionPartyRecord(g_session, 0), PartyFieldAbilities, 0xF000);
        ExploreReveal revealed;
        exploreRevealAroundPlayer(&g_session->save, g_session->x, g_session->y, g_session->facing, &revealed);
    }
    GameUiHost uiHost = {NULL, playEffect, playMusic, playIntro, storeSave, fetchSave, slotName, slotWrite, slotRead, introSoundBusy};
    if (!gameUiInit(&g_ui, g_session, g_worldDat, g_worldSize, pictureFileGet, g_pictures, &uiHost)) {
        fprintf(stderr, "the world data lacks the view tables\n");
        return 1;
    }
    if (loadExe(game)) {
        gameUiSetExe(&g_ui, &g_exe);
        g_session->shareware = opt("EXPLORE_SHAREWARE") != NULL;
        g_ui.clueRegistered = opt("EXPLORE_SHAREWARE") == NULL; /* the registered program (there is no shareware / registered distinction any more): every entry is open; EXPLORE_SHAREWARE=1 brings the original shareware lock back */
    }
    if (loadExe(game)) {
        gameUiSetExe(&g_ui, &g_exe);
    }
    g_ui.debugMarkers = opt("EXPLORE_DEBUG") != NULL;
    g_ui.freeMaps = opt("EXPLORE_FREEMAPS") != NULL; /* M and W without the map item and the Mapping skill */
    if (opt("EXPLORE_LOAD") && !gameUiLoadSlot(&g_ui, (unsigned)atoi(opt("EXPLORE_LOAD")) - 1)) { /* EXPLORE_LOAD=n: start from the game's save slot n (SAVGAMEn) */
        fprintf(stderr, "could not load save slot %s%c", opt("EXPLORE_LOAD"), 10);
    }
    if (opt("EXPLORE_START")) { /* x,y,N|E|S|W: start somewhere else (testing) */
        int sx, sy;
        char dirChar = 'N';
        if (sscanf(opt("EXPLORE_START"), "%d,%d,%c", &sx, &sy, &dirChar) >= 2) {
            g_session->x = sx;
            g_session->y = sy;
            g_session->facing = dirChar == 'S' ? SaveFacingSouth : dirChar == 'E' ? SaveFacingEast : dirChar == 'W' ? SaveFacingWest : SaveFacingNorth;
            sessionRebuild(g_session);
        }
    }
    if (opt("EXPLORE_BOOT")) { /* the original's start: the dialog with LOAD / NEW GAME / DOS when there are saves, else the title menu of a new game (EXPLORE_SPLASH: the boot title and the introduction first) */
        gameUiBoot(&g_ui);
    } else if (opt("EXPLORE_TITLE")) { /* start on the title menu: E enters, I plays the opening, R the story */
        gameUiShowTitle(&g_ui);
    }
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
    unsigned colourStamp = g_ui.paletteStamp;
    for (unsigned i = 0; i < 256; i++) {
        unsigned r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63, b = palette[i * 3 + 2] & 63;
        colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
    }
    if (!opt("EXPLORE_NOSOUND")) {
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

    if (opt("EXPLORE_SPLASH")) { /* the boot title, then the introduction (Escape in the boot title skips it as well: InitGame); Escape in the introduction goes on to the dialog */
        playIntro(NULL, "splash");
        if (!g_introEscape) {
            playIntro(NULL, "opening");
        }
    }
    if (opt("EXPLORE_INTRO") && !playIntro(NULL, opt("EXPLORE_INTRO"))) {
        return 1;
    }
    bool running = !g_quit, dirty = true;
    const char *script = opt("EXPLORE_KEYS");
    static char randomScript[20001];
    if (opt("EXPLORE_RANDOM")) { /* EXPLORE_RANDOM=<n>: n random keys (a soak test; EXPLORE_SHOT still writes the last screen) */
        unsigned n = (unsigned)atoi(opt("EXPLORE_RANDOM"));
        const char alphabet[] = "FFFFFFBLRLRQEAAATCSS";
        for (unsigned i = 0; i < n && i < sizeof(randomScript) - 1; i++) {
            randomScript[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
        }
        script = randomScript;
    }
    const char *shotPath = opt("EXPLORE_SHOT");
    Uint32 lastMinute = 0;
    enum { DoubleClickMs = 330 }; /* six BIOS ticks */
    bool pendingClick = false;
    Uint32 pendingAt = 0;
    int pendingX = 0, pendingY = 0;
    unsigned clockMs = opt("EXPLORE_CLOCK_MS") ? (unsigned)atoi(opt("EXPLORE_CLOCK_MS")) : script ? 0 : 5000; /* the original's clock: a game minute every 91 BIOS ticks, five seconds */ /* real milliseconds per game minute; 0 = the clock only moves with + / - and resting */
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
            if (opt("EXPLORE_TICKS")) { /* EXPLORE_TICKS=n: n animation ticks pass first (a shot in flight, a pressed button) */
                for (int tick = atoi(opt("EXPLORE_TICKS")); tick > 0; tick--) {
                    gameUiAnimate(&g_ui);
                }
            } else if (!opt("EXPLORE_DOORFRAME")) { /* EXPLORE_DOORFRAME=1: the picture of a door that is still opening */
                gameUiSettle(&g_ui); /* otherwise it is passed before the picture */
            }
            const char *clicks = opt("EXPLORE_CLICKS"); /* EXPLORE_CLICKS="x,y;x,y": left clicks at those screen pixels (320 x 200) after the keys, before the picture ('r' as a prefix: the right button) */
            while (clicks && *clicks) {
                bool right = *clicks == 'r', twice = *clicks == 'd'; /* 'd' as a prefix: a double click */
                clicks += right || twice;
                int cx = 0, cy = 0, used = 0;
                if (sscanf(clicks, "%d,%d%n", &cx, &cy, &used) < 2) {
                    break;
                }
                if (!twice || !gameUiDoubleClick(&g_ui, cx, cy)) {
                    gameUiClick(&g_ui, cx, cy, right);
                }
                clicks += used;
                clicks += *clicks == ';';
            }
            gameUiDraw(&g_ui);
            writePng(shotPath, g_ui.screen, palette, ViewScreenWidth, ViewScreenHeight, 2);
            if (g_session->log[0]) {
                printf("log: %s\n", g_session->log);
            }
            printf("(%d, %d) facing %u clock %u -> %s\n", g_session->x, g_session->y, (unsigned)g_session->facing, g_session->clock.minutes, shotPath);
            break;
        }
        SDL_Event event;
        bool cursorMoved = false; /* only the pointer moved, with an item or the crosshair on it: the cursor is redrawn, not the screen */
        /* wait for the first event, then take what else is queued without waiting again (waiting 100 ms for each "next" event held every picture back by that long) */
        for (bool haveEvent = SDL_WaitEventTimeout(&event, script && *script ? 0 : 100); haveEvent; haveEvent = SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                UiKey key = uiKeyFor(event.key.keysym.sym);
                if (key != UiKeyNone) {
                    running = gameUiKey(&g_ui, key, (event.key.keysym.mod & KMOD_CTRL) != 0) && running;
                }
                dirty = true;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)) {
                int winW, winH;
                SDL_GetWindowSize(window, &winW, &winH);
                if (winW > 0 && winH > 0) { /* window pixels -> the 320 x 200 screen */
                    int mx = event.button.x * ViewScreenWidth / winW, my = event.button.y * ViewScreenHeight / winH;
                    bool right = event.button.button == SDL_BUTTON_RIGHT;
                    gameUiMouseMove(&g_ui, mx, my);
                    if (!right && pendingClick && SDL_GetTicks() - pendingAt < DoubleClickMs) { /* the second press of a double click replaces the first (PollKeyboardInput: errorCode 6) */
                        pendingClick = false;
                        running = (gameUiDoubleClick(&g_ui, mx, my) || gameUiClick(&g_ui, mx, my, false)) && running;
                    } else if (!right && gameUiWantsDoubleClick(&g_ui, mx, my)) { /* held back for six ticks, as the original does with every left click */
                        pendingClick = true;
                        pendingAt = SDL_GetTicks();
                        pendingX = mx;
                        pendingY = my;
                    } else {
                        running = gameUiClick(&g_ui, mx, my, right) && running;
                    }
                }
                dirty = true;
            } else if (event.type == SDL_MOUSEMOTION) { /* the cursor item follows the pointer */
                int winW, winH;
                SDL_GetWindowSize(window, &winW, &winH);
                if (winW > 0 && winH > 0) {
                    gameUiMouseMove(&g_ui, event.motion.x * ViewScreenWidth / winW, event.motion.y * ViewScreenHeight / winH);
                    cursorMoved = cursorMoved || gameUiCursorShown(&g_ui);
                }
            } else if (event.type == SDL_WINDOWEVENT) {
                dirty = true;
            }
        }
        if (pendingClick && SDL_GetTicks() - pendingAt >= DoubleClickMs) {
            pendingClick = false;
            running = gameUiClick(&g_ui, pendingX, pendingY, false) && running;
            dirty = true;
        }
        SDL_ShowCursor(gameUiCursorShown(&g_ui) ? SDL_DISABLE : SDL_ENABLE); /* the screen draws the cursor item or the crosshair itself */
        if (clockMs && !g_session->combat.active && g_ui.overlay == UiOverlayNone && SDL_GetTicks() - lastMinute >= clockMs) {
            lastMinute = SDL_GetTicks();
            sessionAdvanceTime(g_session, 1);
            dirty = true;
        }
        static Uint32 lastAnimation;
        if (!script && SDL_GetTicks() - lastAnimation >= 110) { /* the animation tick: two BIOS clock counts */
            lastAnimation = SDL_GetTicks();
            dirty = gameUiAnimate(&g_ui) || dirty;
        }
        bool cursorOnly = false;
        if (!dirty && cursorMoved) {
            gameUiRedrawCursor(&g_ui);
            dirty = cursorOnly = true;
        } else if (dirty) {
            gameUiDraw(&g_ui);
        }
        if (dirty) {
            if (colourStamp != g_ui.paletteStamp) { /* the cycled colours (gameUiPalette) moved on */
                colourStamp = g_ui.paletteStamp;
                for (unsigned i = 0; i < 0x100; i++) { /* the cycled colours and the sky, and whatever a fade (the clue book's) has done to the rest */
                    unsigned r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63, b = palette[i * 3 + 2] & 63;
                    colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
                }
            }
            static uint32_t pixels[ViewScreenWidth * ViewScreenHeight];
            for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
                pixels[i] = colours[g_ui.screen[i]];
            }
            SDL_UpdateTexture(texture, NULL, pixels, ViewScreenWidth * 4);
            SDL_RenderClear(sdl);
            SDL_RenderCopy(sdl, texture, NULL, NULL);
            SDL_RenderPresent(sdl);
            if (!cursorOnly) {
                char title[256];
                gameUiStatus(&g_ui, title, sizeof(title));
                SDL_SetWindowTitle(window, title);
            }
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

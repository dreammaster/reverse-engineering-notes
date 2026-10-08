/*
 * An interactive walk through a real map: the game core (session.h) driven by a keyboard in an SDL2 window. Everything that is game logic -- movement and
 * passability, the fog-of-war reveal, the monsters and the combat, resting, locks and chests -- is in the session; this file only reads keys, draws the screen
 * (the first-person view with the day/night lighting, the minimap, the four party panels, the local area map, the pause dialog, the hero sheets and paper dolls)
 * and plays sound (the area music through the OPL2 synthesizer, a couple of effects). It is the smallest "engine" built from the modules.
 *
 * Build and run (from src23/tools; SDL2 from C:\sdk\SDL2-2.32.10, SDL2.dll next to the exe or on PATH):
 *   gcc -Wall -Wextra -std=c99 -I .. -I /c/sdk/SDL2-2.32.10/include -o explore_sdl explore_sdl.c ../session.c ../windowbake.c ../interact.c ../lockcatalog.c \
 *       ../worldobjects.c ../monsterpool.c ../monster.c ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../cmfplayer.c ../opl.c ../cmf.c \
 *       ../audio.c ../voc.c ../music.c ../spellrecord.c ../paperdoll.c ../statsheet.c ../roster.c ../viewrender.c ../minimap.c ../statuspanel.c ../font.c \
 *       ../uiregions.c ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c \
 *       ../introstory.c ../intro2.c ../palettefade.c ../exedata.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c ../conversation.c ../dialog.c ../dialogservice.c ../shop.c ../spellcast.c ../consumable.c \
 *       ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../palette.c \
 *       -L /c/sdk/SDL2-2.32.10/lib -lmingw32 -lSDL2main -lSDL2 -lm
 *   ./explore_sdl <2|3> <game dir> [scale]
 *
 * Keys: Up / Down walk, Left / Right turn, Ctrl+Left / Ctrl+Right strafe (the original's scan codes, through mainCommandForKey), A attacks while a combat is on (C casts the first attack spell the member can, and outside a combat the next support / exploration spell any member can: the party starts with none of the ore the spells cost, so loot a chest first),
 * R rest (eight hours; monsters can interrupt it), K unlock the door ahead with a skeleton key, S use the object ahead (loot the chest, take a road or portal, play the Chapter 2 ending at the Quartz Chamber), M the local area map, D the pause
 * dialog, P the paper dolls, F1-F4 the hero's detail sheet, + / - move the clock by 30 minutes (watch the lighting), Escape closes an overlay or quits.
 *
 * EXPLORE_INTRO=opening|story|ending plays a cinematic of the game first (opening = the opening and the story; Chapter 3 has only its opening; EXPLORE_INTRO_MS sets
 * the milliseconds per tick, 25 by default; EXPLORE_INTRO_SOUND=1 plays the voices instead of drawing the text); Escape ends it.
 *
 * Headless check: with EXPLORE_KEYS set (F B L R forward / back / turn left / turn right, Q E strafe, M map, T rest, D dialog, K, S, A, P, 1 2, + -, ESC as '!') the
 * keys are played at start and the final screen is written to the PNG named by EXPLORE_SHOT, then the program exits (SDL_VIDEODRIVER=dummy needs no display;
 * SDL_AUDIODRIVER=dummy or EXPLORE_NOSOUND=1 silences it). EXPLORE_RANDOM=<n> plays n random keys, EXPLORE_START=x,y,N|E|S|W starts elsewhere, EXPLORE_DEBUG=1
 * lists the monster markers in view.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "audio.h"
#include "cmfplayer.h"
#include "exedata.h"
#include "font.h"
#include "gamedialog.h"
#include "introstory.h"
#include "lighting.h"
#include "localmap.h"
#include "maininput.h"
#include "minimap.h"
#include "music.h"
#include "paperdoll.h"
#include "pictures_stdio.h"
#include "pngwrite.h"
#include "session.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "voc.h"

typedef enum { OverlayNone, OverlayLocalMap, OverlayDialog, OverlayDetail, OverlayDolls, OverlayTalk } Overlay;

static GameSession *g_session;
static uint8_t g_screen[ViewScreenWidth * ViewScreenHeight];
static uint8_t g_tables[ViewTablesSize];
static unsigned g_overlayHero;

/* Sound: the area music through the OPL2 synthesizer (opl.c), effects as VOC samples, both mixed in SDL's audio callback. */
static uint8_t g_worldDat[5000000];
static size_t g_worldSize;
static CmfPlayer g_music;
static bool g_musicOn;
static unsigned g_musicTrack, g_lastMusicPage = 0xFFFF;
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

static void playMusic(unsigned track) {
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
static void playEffect(unsigned id) {
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

static void drawTalk(void);

static void drawScene(ViewRenderer *renderer, PictureFile *pictures, Overlay overlay) {
    GameSession *s = g_session;
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    if (getenv("EXPLORE_DEBUG")) {
        for (unsigned i = 0; i < ViewportCellCount; i++) {
            if (cells[i].flags & DungeonGridCellFlagOverlay) {
                fprintf(stderr, "view cell %u: marker type %u flags %04X monster %s\n", i, (unsigned)cells[i].reserved4, (unsigned)cells[i].flags, scene.cellMonsters[i] ? "yes" : "no");
            }
        }
    }
    memset(g_screen, 0, sizeof(g_screen));
    const uint8_t *frame = pictureFileGet(pictures, 0, 1);
    for (unsigned row = 0; frame && row < 198; row++) {
        memcpy(&g_screen[(row + 1) * ViewScreenWidth + 1], &frame[row * 318], 318);
    }
    viewRender(renderer, &scene);
    MinimapTile tiles[MinimapCells];
    int16_t shades[LightingViewportCells];
    lightingViewportTable(scene.gradient, shades);
    minimapBuild(s->game, &s->grid, s->x, s->y, tiles);
    minimapDraw(renderer, tiles, shades, s->facing);
    for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
        statusPanelDraw(renderer, panel, sessionPartyRecord(s, panel));
    }
    if (overlay == OverlayLocalMap) {
        static LocalMapCell localCells[LocalMapColumns * LocalMapRows];
        int c0, r0;
        localMapBlockOrigin(s->x, s->y, &c0, &r0);
        localMapFill(localCells, s->game, &s->map, &s->save, c0, r0);
        memset(g_screen, 0, sizeof(g_screen));
        localMapDraw(renderer, localCells, s->x, s->y, s->facing);
    } else if (overlay == OverlayDetail) {
        uint16_t roles[5];
        for (unsigned i = 0; i < 5; i++) {
            roles[i] = saveHeaderGetU16(&s->save, SaveHeaderRoleAssignments + 2 * i);
        }
        detailSheetDraw(renderer, sessionPartyRecord(s, g_overlayHero), 6 + g_overlayHero, roles);
    } else if (overlay == OverlayTalk) {
        memset(g_screen, 0, sizeof(g_screen));
        drawTalk();
    } else if (overlay == OverlayDolls) {
        for (unsigned i = 0; i < 4; i++) {
            paperDollDraw(renderer, &s->items, sessionPartyRecord(s, i), 8 + 56 * (int)i, 8);
        }
    } else if (overlay == OverlayDialog) {
        gameDialogDraw(renderer, GameDialogFlagReturn | GameDialogFlagAnimation | GameDialogFlagSave | GameDialogFlagLoad | GameDialogFlagNewGame,
                       DriverMusicAvailable | DriverSoundFxAvailable | DriverMusicOn, 5);
    }
}

static void area(void) {
    GameSession *s = g_session;
    uint16_t track;
    if (musicRegionChanged(&g_lastMusicPage, s->x, s->y, s->game, g_worldDat, g_worldSize, &track)) {
        playMusic(track);
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
    default: return SDLK_UNKNOWN;
    }
}

/* One key press: returns false to quit. */
static bool playIntro(const char *kind, const char *dir, PictureFile *pictures, SDL_Renderer *renderer, SDL_Texture *texture);
static const char *g_dir;
static PictureFile *g_pictures;
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;

/* The use command on the object ahead (sessionInteract): chests are looted, destinations travelled to, the Chapter 2 ending is played; the rest shows up in the window title. */
static char g_talkText[1200];

/* The NPC's text (the '%' marks a line break; ~ is the font's apostrophe), wrapped at 49 characters, and the topics that can be chosen. */
static void drawTalk(void) {
    GameSession *s = g_session;
    int y = 10;
    char line[64];
    unsigned used = 0;
    for (const char *p = g_talkText;; p++) { /* greedy word wrap at 48 characters; '%' forces a break */
        bool end = *p == 0, brk = *p == '%', space = *p == ' ';
        if (!end && !brk && !(space && used == 0)) {
            line[used++] = *p == '~' ? '\'' : *p;
        }
        bool wrap = false;
        if (!end && !brk && used >= 46 && space) {
            wrap = true;
        } else if (used >= 52) {
            wrap = true;
        }
        if (end || brk || wrap) {
            while (used > 0 && line[used - 1] == ' ') {
                used--;
            }
            line[used] = 0;
            if (used) {
                fontDrawString(s->game, 0, g_screen, ViewScreenWidth, 8, y, line, 0x8A, 0, FontTransparent);
                y += 7;
            }
            used = 0;
        }
        if (end) {
            break;
        }
    }
    y = 120;
    for (unsigned i = 0; i < s->talk.listedCount; i++) {
        char name[DialogTopicNameSize + 1], row[32];
        dialogTopicName(dialogTopic(&s->dialogs, s->talk.listed[i]), name);
        snprintf(row, sizeof(row), "%u %s", i + 1, name);
        fontDrawString(s->game, 0, g_screen, ViewScreenWidth, 8 + (int)(i / 5) * 150, y + (int)(i % 5) * 8, row, 0x9B, 0, FontTransparent);
    }
}

static void talkStep(const ConversationStep *step) {
    GameSession *s = g_session;
    if (step->topic) {
        conversationText(&s->talk, step->topic, g_talkText, sizeof(g_talkText));
        SessionService service = sessionTalkService(s, step, NULL);
        if (service == SessionServiceNone && step->needsResolve) {
            sessionTalkResolve(s, true);
        }
    }
}

static void useAhead(Overlay *overlay) {
    GameSession *s = g_session;
    SessionInteract use = sessionInteract(s);
    switch (use.kind) {
    case SessionInteractDoor:
        sessionLoot(s);
        break;
    case SessionInteractTravelled:
    case SessionInteractStoryEvent:
        area();
        break;
    case SessionInteractTravelRefused:
        snprintf(s->log, sizeof(s->log), "the way is closed (message %u)", use.messageId);
        break;
    case SessionInteractTravelPassword:
        snprintf(s->log, sizeof(s->log), "this way asks for a password (prompt %u)", use.promptId);
        break;
    case SessionInteractTalker: {
        ConversationStep first;
        if (sessionTalkStart(s, use.value, &first)) {
            talkStep(&first);
            *overlay = OverlayTalk;
        }
        break;
    }
    case SessionInteractStoryRefused:
    case SessionInteractEndingNotReady:
        snprintf(s->log, sizeof(s->log), "nothing happens (yet)");
        break;
    case SessionInteractEnding:
        playIntro("ending", g_dir, g_pictures, g_renderer, g_texture);
        s->clock.minutes = 480;
        break;
    default:
        break;
    }
    if (s->pendingSound) {
        playEffect(s->pendingSound);
        s->pendingSound = 0;
    }
}

static bool handleKey(SDL_Keycode key, bool ctrl, Overlay *overlay) {
    GameSession *s = g_session;
    if (*overlay == OverlayTalk) { /* 1-9 choose a topic, Escape the last one listed (the way out) */
        unsigned number = key == SDLK_ESCAPE ? 0 : key >= SDLK_1 && key <= SDLK_9 ? (unsigned)(key - SDLK_1 + 1) : 99;
        if (number != 99) {
            ConversationStep step = sessionTalkChoose(s, number);
            talkStep(&step);
            if (!s->talking) {
                *overlay = OverlayNone;
            }
        }
        return true;
    }
    if (key == SDLK_ESCAPE) {
        if (*overlay != OverlayNone) {
            *overlay = OverlayNone;
            return true;
        }
        return false;
    }
    if (key == SDLK_PLUS || key == SDLK_EQUALS || key == SDLK_KP_PLUS) {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 30) % 1440);
        return true;
    }
    if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 1440 - 30) % 1440);
        return true;
    }
    if (*overlay != OverlayNone) {
        return true;
    }
    if (s->combat.active) {
        if (key == SDLK_a) {
            sessionAttack(s);
        } else if (key == SDLK_c) { /* the first attack spell the member can cast */
            for (unsigned spell = 1; spell < 126; spell++) {
                if (sessionCast(s, spell) == SessionCastDone) {
                    break;
                }
            }
        }
        return true;
    }
    if (key == SDLK_c) { /* outside a combat: the next spell, in id order, that any member knows and can cast (support and exploration spells) */
        static unsigned next = 1;
        for (unsigned tried = 0; tried < 126; tried++, next = next % 125 + 1) {
            bool cast = false;
            for (unsigned member = 0; member < 4 && !cast; member++) {
                SessionCast result = sessionCastBy(s, member, next, false);
                cast = result == SessionCastDone || result == SessionCastNoEffect;
            }
            if (cast) {
                next = next % 125 + 1;
                area();
                return true;
            }
        }
        return true;
    }
    bool extended = true;
    uint8_t code = 0;
    if (key == SDLK_UP) {
        code = 0x48;
    } else if (key == SDLK_DOWN) {
        code = 0x50;
    } else if (key == SDLK_LEFT) {
        code = ctrl ? 0x73 : 0x4B;
    } else if (key == SDLK_RIGHT) {
        code = ctrl ? 0x74 : 0x4D;
    } else if (key >= SDLK_F1 && key <= SDLK_F4) {
        code = (uint8_t)(0x3B + (key - SDLK_F1));
    } else if (key >= SDLK_a && key <= SDLK_z) {
        extended = false;
        code = (uint8_t)(key - SDLK_a + 'A');
    }
    MainCommand command = mainCommandForKey(s->game, extended, code);
    switch (command.action) {
    case MainActionMove:
        if (sessionMove(s, command.movement) == SessionStepBlocked && s->game == GameYendor2) {
            playEffect(6); /* the bump sound, _val33 */
        }
        area();
        break;
    case MainActionRest:
        sessionRest(s);
        break;
    case MainActionUnlockDoor: /* a demo skeleton key: every tier, as chest and door key at once */
        sessionUnlock(s, 0xFFC0, 0);
        break;
    case MainActionAct: /* use the object ahead: loot the chest, take the road, ... */
        useAhead(overlay);
        break;
    case MainActionLocalMap:
        *overlay = OverlayLocalMap;
        break;
    case MainActionGameDialog:
        *overlay = OverlayDialog;
        break;
    case MainActionMemberDetail:
        g_overlayHero = command.index & 3;
        *overlay = OverlayDetail;
        break;
    case MainActionPartyInventory:
        *overlay = OverlayDolls;
        break;
    default:
        break;
    }
    return true;
}

/* ---- the cinematics (introstory.h): EXPLORE_INTRO=opening|story|ending plays one in the window before the game; Escape ends it --------------------------------- */
static SDL_Renderer *g_introRenderer;
static SDL_Texture *g_introTexture;
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

static void introSound(void *ctx, unsigned id) {
    (void)ctx;
    playEffect(id);
}

static void introMusic(void *ctx, unsigned track) {
    (void)ctx;
    playMusic(track);
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

static bool playIntro(const char *kind, const char *dir, PictureFile *pictures, SDL_Renderer *renderer, SDL_Texture *texture) {
    GameKind game = g_session->game;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, game == GameYendor3 ? "REGISTER.EXE" : "SW.EXE");
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
    IntroAssets assets = {g_worldDat, g_worldSize, &exe, pictureFileGet, pictures, game};
    static IntroStory story;
    g_introRenderer = renderer;
    g_introTexture = texture;
    if (getenv("EXPLORE_INTRO_MS")) {
        g_introDelayMs = (unsigned)atoi(getenv("EXPLORE_INTRO_MS"));
    }
    IntroHost host = {NULL, introPresent, introSound, introMusic, introTick, introEscapeFn, NULL, getenv("EXPLORE_INTRO_SOUND") != NULL, introWaitSound, introSoundBusy};
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
    playMusic(0);
    return ok || g_introEscape;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <2|3> <game dir> [scale]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    const char *dir = argv[2];
    int scale = argc > 3 ? atoi(argv[3]) : 3;
    if (scale < 1) {
        scale = 3;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    static uint8_t palette[PicturePaletteSize];
    FILE *f = fopen(path, "rb");
    g_worldSize = f ? fread(g_worldDat, 1, sizeof(g_worldDat), f) : 0;
    if (f) {
        fclose(f);
    }
    if (!g_worldSize || viewTablesOffset(game) + sizeof(g_tables) > g_worldSize || pictureMasterPaletteOffset(game) + sizeof(palette) > g_worldSize) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    memcpy(g_tables, g_worldDat + viewTablesOffset(game), sizeof(g_tables));
    memcpy(palette, g_worldDat + pictureMasterPaletteOffset(game), sizeof(palette));
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir);
    PictureFile *pictures = pictureFileOpen(path, game);
    if (!pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    g_session = sessionNew(game, g_worldDat, g_worldSize);
    if (!g_session) {
        fprintf(stderr, "cannot start a session\n");
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
    uint32_t colours[256];
    for (unsigned i = 0; i < 256; i++) {
        unsigned r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63, b = palette[i * 3 + 2] & 63;
        colours[i] = 0xFF000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((b << 2) | (b >> 4));
    }
    ViewRenderer renderer = {game, g_tables, pictureFileGet, pictures, g_screen, NULL};
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
            area();
            SDL_PauseAudioDevice(g_audio, 0);
        }
    }

    if (getenv("EXPLORE_INTRO") && !playIntro(getenv("EXPLORE_INTRO"), dir, pictures, sdl, texture)) {
        return 1;
    }
    g_dir = dir;
    g_pictures = pictures;
    g_renderer = sdl;
    g_texture = texture;
    Overlay overlay = OverlayNone;
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
            drawScene(&renderer, pictures, overlay);
            writePng(shotPath, g_screen, palette, ViewScreenWidth, ViewScreenHeight, 2);
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
                running = handleKey(event.key.keysym.sym, (event.key.keysym.mod & KMOD_CTRL) != 0, &overlay) && running;
                dirty = true;
            } else if (event.type == SDL_WINDOWEVENT) {
                dirty = true;
            }
        }
        if (dirty) {
            drawScene(&renderer, pictures, overlay);
            static uint32_t pixels[ViewScreenWidth * ViewScreenHeight];
            for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
                pixels[i] = colours[g_screen[i]];
            }
            SDL_UpdateTexture(texture, NULL, pixels, ViewScreenWidth * 4);
            SDL_RenderClear(sdl);
            SDL_RenderCopy(sdl, texture, NULL, NULL);
            SDL_RenderPresent(sdl);
            char title[256];
            snprintf(title, sizeof(title), "Yendorian Tales - (%d, %d) clock %02u:%02u%s%s%s", g_session->x, g_session->y, g_session->clock.minutes / 60,
                     g_session->clock.minutes % 60, g_session->combat.active ? " - COMBAT: A attacks" : "", g_session->log[0] ? " - " : "", g_session->log);
            SDL_SetWindowTitle(window, title);
            dirty = false;
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    sessionFree(g_session);
    pictureFileClose(pictures);
    return 0;
}

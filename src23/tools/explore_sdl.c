/*
 * An interactive walk through a real map: the decoded modules driven by a keyboard, in an SDL2 window -- movement and passability, the fog-of-war
 * reveal, the first-person view with the day/night lighting, the minimap, the four party panels, the local area map (M) and the pause dialog (D).
 * It is the smallest "engine" that uses them together; nothing here is game logic (that is all in the modules), only input and display glue.
 *
 * Build and run (from src23/tools; SDL2 from C:\sdk\SDL2-2.32.10, SDL2.dll next to the exe or on PATH):
 *   gcc -Wall -Wextra -std=c99 -I .. -I /c/sdk/SDL2-2.32.10/include -o explore_sdl explore_sdl.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c \
 *       ../monsterpool.c ../monster.c ../monster_stdio.c ../globalflags.c ../cmfplayer.c ../opl.c ../cmf.c ../audio.c ../voc.c ../music.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../item_stdio.c ../spellrecord.c ../viewrender.c ../minimap.c ../statuspanel.c ../font.c ../uiregions.c \
 *       ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c \
 *       ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../palette.c \
 *       -L /c/sdk/SDL2-2.32.10/lib -lmingw32 -lSDL2main -lSDL2 -lm
 *   ./explore_sdl <2|3> <game dir> [scale]
 *
 * Keys: Up / Down walk, Left / Right turn, Ctrl+Left / Ctrl+Right strafe (the original's scan codes, through mainCommandForKey), M the local area
 * map, R rest (eight hours; monsters can interrupt it), K unlock the door ahead with a skeleton key, S loot the chest ahead, D the pause dialog, P the paper dolls, F1-F4 the hero's detail sheet, + / - move the clock by 30 minutes (watch the lighting), Escape closes an overlay or quits.
 *
 * Headless check: with EXPLORE_KEYS set (F B L R forward / back / turn left / turn right, Q E strafe, M map, T rest, D dialog, + -, ESC as '!') the keys are played
 * at start and the final screen is written to the PNG named by EXPLORE_SHOT, then the program exits (SDL_VIDEODRIVER=dummy needs no display).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "audio.h"
#include "chest.h"
#include "cmfplayer.h"
#include "combat.h"
#include "dungeongrid.h"
#include "explore.h"
#include "gamedialog.h"
#include "lighting.h"
#include "localmap.h"
#include "item_stdio.h"
#include "maininput.h"
#include "minimap.h"
#include "music.h"
#include "monster_stdio.h"
#include "monsterpool.h"
#include "movement.h"
#include "newgame.h"
#include "paperdoll.h"
#include "pictures_stdio.h"
#include "rest.h"
#include "pngwrite.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "viewport.h"
#include "viewrender.h"
#include "voc.h"
#include "windowbake.h"
#include "worldmap_stdio.h"

typedef enum { OverlayNone, OverlayLocalMap, OverlayDialog, OverlayDetail, OverlayDolls } Overlay;
static unsigned g_overlayHero;

static WorldMap g_map;
static WorldObjectTable g_objects;
static LockCatalog g_locks;
static MonsterCatalog g_catalog;
static uint8_t g_pool[MonsterPoolSize * MonsterRecordSize];
static RandomState g_rng;
static int g_engagedType = 0;
static ItemCatalog g_items;
static GameKind g_game;

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
            unsigned track = g_musicTrack;
            uint32_t offset, length;
            if (audioMusicTrack(g_game, track, &offset, &length) && offset + length <= g_worldSize) {
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
    g_musicOn = track && audioMusicTrack(g_game, track, &offset, &length) && offset + length <= g_worldSize && cmfPlayerStart(&g_music, g_worldDat + offset, length, 44100);
    SDL_UnlockAudioDevice(g_audio);
}

/* Sound effect id (WORLD.DAT's VOC list) resampled to 44.1 kHz. */
static void playEffect(unsigned id) {
    uint32_t offset, length;
    VocSound voc;
    if (!g_audio || !audioEffect(g_game, id, &offset, &length) || offset + length > g_worldSize || !vocOpen(&voc, g_worldDat + offset, length)) {
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

/* The combat of the original's main loop (RunDungeonGameLoop): up to three monsters in slots, a turn order by dexterity, and per tick the current combatant acts. */
static struct {
    bool active, wiped;
    uint8_t slots[CombatMonsterSlotCount * MonsterRecordSize];
    bool defeated[CombatMonsterSlotCount];
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    unsigned count, cursor;
    uint16_t targets[CombatMonsterSlotCount];
    MonsterRewardStaging staging;
    char log[96];
} g_combat;
static SaveGame g_save;
static DungeonGrid g_grid;
static uint8_t g_screen[ViewScreenWidth * ViewScreenHeight];
static uint8_t g_tables[ViewTablesSize];

/* RefreshDungeonMapWindow: the base window, the interaction markers baked in, then the live monsters relinked into it. */
static void buildWindow(GameKind game, int x, int y) {
    dungeonGridBuild(&g_grid, game, &g_map, &g_save, x, y);
    dungeonGridBakeMarkers(&g_grid, game, &g_objects, &g_locks, &g_save);
    monsterPoolRefreshWindow(g_pool, &g_grid, &g_save);
}


static uint8_t *partySlotRecord(unsigned slot) {
    unsigned id = saveHeaderGetU16(&g_save, SaveHeaderPartySlots + 2 * slot);
    return id ? saveGamePartyRecordById(&g_save, id) : NULL;
}

static void combatNewRound(void) {
    g_combat.count = combatBuildTurnOrder(&g_save, g_combat.slots, &g_rng, g_combat.order, g_combat.targets);
    g_combat.cursor = 0;
}

static void combatCheckWipe(void) {
    const uint8_t *records[4];
    for (unsigned i = 0; i < 4; i++) {
        records[i] = partySlotRecord(i);
    }
    if (partyWipedOut(records)) {
        g_combat.wiped = true;
        g_combat.active = false;
        snprintf(g_combat.log, sizeof(g_combat.log), "THE PARTY HAS BEEN DEFEATED");
    }
}

/* After a combatant has acted: reap the dead, then move to the next one (ProcessCombatRound). */
static void combatAfterAction(void) {
    CombatRoundOutcome outcome = combatProcessRound(g_combat.slots, g_combat.order, g_combat.count, g_combat.defeated, &g_combat.cursor, &g_combat.staging, NULL, 0);
    if (outcome == CombatRoundNoMonstersLeft) {
        monsterRewardsAward(&g_save, g_game, &g_combat.staging);
        memset(&g_combat.staging, 0, sizeof(g_combat.staging));
        g_combat.active = false;
        g_engagedType = 0;
        snprintf(g_combat.log, sizeof(g_combat.log), "VICTORY - the loot and experience are in the party's totals");
    } else if (outcome == CombatRoundNewRound) {
        combatNewRound();
    }
}

/* Runs monster turns until it is a party member's turn (or the combat ends). */
static void combatRunMonsters(void) {
    while (g_combat.active && g_combat.cursor < g_combat.count && g_combat.order[g_combat.cursor].isMonster) {
        unsigned slot = g_combat.order[g_combat.cursor].index;
        uint8_t *monster = g_combat.slots + (size_t)slot * MonsterRecordSize;
        uint16_t targetId = g_combat.targets[slot];
        uint8_t *target = targetId ? saveGamePartyRecordById(&g_save, targetId) : NULL;
        CombatMonsterTurnOutcome out = combatProcessMonsterTurn(monster, target, &g_save, &g_items, g_game, &g_rng);
        snprintf(g_combat.log, sizeof(g_combat.log), "monster %u: %s", (unsigned)monsterGetU16(monster, MonsterFieldType), out.attacked ? "hits the party" : "misses / waits");
        combatCheckWipe();
        if (!g_combat.active) {
            return;
        }
        combatAfterAction();
    }
}

static void combatStart(uint8_t *poolRecord) {
    memset(&g_combat, 0, sizeof(g_combat));
    memcpy(g_combat.slots, poolRecord, MonsterRecordSize);
    monsterPoolRemove(poolRecord, &g_grid);
    g_combat.active = true;
    combatNewRound();
    snprintf(g_combat.log, sizeof(g_combat.log), "COMBAT");
    combatRunMonsters();
}

/* The player's A: the current party member swings at the first live monster. */
static void combatPlayerAttack(void) {
    if (!g_combat.active || g_combat.cursor >= g_combat.count || g_combat.order[g_combat.cursor].isMonster) {
        return;
    }
    uint8_t *member = partySlotRecord(g_combat.order[g_combat.cursor].index);
    unsigned monsterSlot = 0;
    if (!member || !combatSelectActiveMonster(g_combat.order, g_combat.count, g_combat.defeated, &monsterSlot)) {
        return;
    }
    CombatPlayerMeleeOutcome out = combatPlayerMeleeAttack(member, g_combat.slots + (size_t)monsterSlot * MonsterRecordSize, &g_items, g_game, &g_rng);
    snprintf(g_combat.log, sizeof(g_combat.log), "party member %u: %s", g_combat.order[g_combat.cursor].index + 1,
             out.weaponBroke ? "the weapon broke" : out.hit ? "hits" : "misses");
    combatAfterAction();
    combatRunMonsters();
}

/* ProcessLevelMonsters: every live monster takes its turn (walk toward the party, ambush, timers); one reaching the party engages it. */
static void monstersTakeTurns(GameKind game, int x, int y) {
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    for (unsigned slot = 0; slot < MonsterPoolSize && !g_engagedType; slot++) {
        uint8_t *record = g_pool + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0) {
            continue;
        }
        MonsterFullTurn turn = monsterPoolTakeTurn(record, game, &g_map, &g_grid, NULL, 0, &staging, x, y, &g_rng);
        if (turn.move == MonsterMoveEngaged) {
            g_engagedType = (int)monsterGetU16(record, MonsterFieldType);
            combatStart(record);
        }
    }
}

static bool restMonstersTurn(void *ctx) {
    const int *pos = ctx;
    monstersTakeTurns(g_game, pos[0], pos[1]);
    return g_combat.active;
}

static void drawScene(GameKind game, ViewRenderer *renderer, PictureFile *pictures, int x, int y, uint16_t facing, unsigned clock, Overlay overlay) {
    buildWindow(game, x, y);
    DungeonGridCell cells[ViewportCellCount];
    viewportBuild(&g_grid, facing, x, y, cells);
    viewportComputeVisibility(game, cells);
    LightingInput light = {0, 0, (uint16_t)clock, facing};
    ViewScene scene;
    memset(&scene, 0, sizeof(scene));
    bool reset;
    lightingComputeGradient(game, &light, 0, scene.gradient, &reset);
    scene.cells = cells;
    scene.facing = facing;
    for (unsigned i = 0; g_combat.active && i < CombatMonsterSlotCount; i++) {
        uint8_t *slotRecord = g_combat.slots + (size_t)i * MonsterRecordSize;
        scene.combatMonsters[i] = monsterGetU16(slotRecord, MonsterFieldType) ? slotRecord : NULL;
    }
    monsterPoolEncounterScan(g_pool, &g_catalog, &g_save, game, facing, (uint16_t)x, (uint16_t)y, (uint16_t)g_grid.originRow, (uint16_t)g_grid.originCol, cells,
                             scene.cellMonsters, &g_rng);
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
    minimapBuild(game, &g_grid, x, y, tiles);
    minimapDraw(renderer, tiles, shades, facing);
    for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
        statusPanelDraw(renderer, panel, saveGamePartyRecord(&g_save, 5 + panel));
    }
    if (overlay == OverlayLocalMap) {
        static LocalMapCell localCells[LocalMapColumns * LocalMapRows];
        int c0, r0;
        localMapBlockOrigin(x, y, &c0, &r0);
        localMapFill(localCells, game, &g_map, &g_save, c0, r0);
        memset(g_screen, 0, sizeof(g_screen));
        localMapDraw(renderer, localCells, x, y, facing);
    } else if (overlay == OverlayDetail) {
        uint16_t roles[5];
        for (unsigned i = 0; i < 5; i++) {
            roles[i] = saveHeaderGetU16(&g_save, SaveHeaderRoleAssignments + 2 * i);
        }
        detailSheetDraw(renderer, saveGamePartyRecord(&g_save, 5 + g_overlayHero), 6 + g_overlayHero, roles);
    } else if (overlay == OverlayDolls) {
        for (unsigned i = 0; i < 4; i++) {
            paperDollDraw(renderer, &g_items, saveGamePartyRecord(&g_save, 5 + i), 8 + 56 * (int)i, 8);
        }
    } else if (overlay == OverlayDialog) {
        gameDialogDraw(renderer, GameDialogFlagReturn | GameDialogFlagAnimation | GameDialogFlagSave | GameDialogFlagLoad | GameDialogFlagNewGame,
                       DriverMusicAvailable | DriverSoundFxAvailable | DriverMusicOn, 5);
    }
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
    if (!worldMapReadWorldDatFile(&g_map, game, path)) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    static uint8_t worldDat[5000000], palette[PicturePaletteSize];
    FILE *f = fopen(path, "rb");
    size_t worldSize = f ? fread(worldDat, 1, sizeof(worldDat), f) : 0;
    if (f) {
        fclose(f);
    }
    if (!worldSize || viewTablesOffset(game) + sizeof(g_tables) > worldSize || pictureMasterPaletteOffset(game) + sizeof(palette) > worldSize) {
        fprintf(stderr, "cannot read the view tables\n");
        return 1;
    }
    memcpy(g_worldDat, worldDat, worldSize);
    g_worldSize = worldSize;
    memcpy(g_tables, worldDat + viewTablesOffset(game), sizeof(g_tables));
    memcpy(palette, worldDat + pictureMasterPaletteOffset(game), sizeof(palette));
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir);
    PictureFile *pictures = pictureFileOpen(path, game);
    if (!pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    if (!worldObjectTableParseWorldDat(&g_objects, game, worldDat, worldSize) || !lockCatalogParseWorldDat(&g_locks, game, worldDat, worldSize) ||
        !monsterCatalogParseWorldDat(&g_catalog, game, worldDat, worldSize)) {
        fprintf(stderr, "cannot read the world objects, locks or monsters\n");
        return 1;
    }
    g_game = game;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    if (!itemCatalogReadWorldDatFile(&g_items, game, path)) {
        fprintf(stderr, "cannot read the item catalog\n");
        return 1;
    }
    randomStart(&g_rng, 30, 50);
    saveGameInit(&g_save, game);
    if (!saveGameNewGame(&g_save, game, worldDat, worldSize)) {
        fprintf(stderr, "cannot build the new game\n");
        return 1;
    }
    for (unsigned slot = 0; slot < 4; slot++) { /* the four ready-made heroes (records 5-8) are the active party */
        saveHeaderSetU16(&g_save, SaveHeaderPartySlots + slot * 2, (uint16_t)(6 + slot));
    }
    int x = saveHeaderGetU16(&g_save, SaveHeaderWorldX), y = saveHeaderGetU16(&g_save, SaveHeaderWorldY);
    uint16_t facing = saveHeaderGetU16(&g_save, SaveHeaderFacing);
    unsigned clock = saveHeaderGetU16(&g_save, SaveHeaderClockMinutes);

    if (getenv("EXPLORE_START")) { /* x,y,N|E|S|W: start somewhere else (testing) */
        int sx, sy;
        char dirChar = 'N';
        if (sscanf(getenv("EXPLORE_START"), "%d,%d,%c", &sx, &sy, &dirChar) >= 2) {
            x = sx;
            y = sy;
            facing = dirChar == 'S' ? SaveFacingSouth : dirChar == 'E' ? SaveFacingEast : dirChar == 'W' ? SaveFacingWest : SaveFacingNorth;
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
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&g_save, x, y, facing, &revealed);
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
            uint16_t track = 0;
            if (musicRegionChanged(&g_lastMusicPage, x, y, game, g_worldDat, g_worldSize, &track)) {
                playMusic(track);
            }
            SDL_PauseAudioDevice(g_audio, 0);
        }
    }

    Overlay overlay = OverlayNone;
    bool running = true, dirty = true;
    const char *script = getenv("EXPLORE_KEYS");
    static char randomScript[20001];
    if (getenv("EXPLORE_RANDOM")) { /* EXPLORE_RANDOM=<n>: n random keys (a soak test; EXPLORE_SHOT still writes the last screen) */
        unsigned n = (unsigned)atoi(getenv("EXPLORE_RANDOM"));
        const char alphabet[] = "FFFFFFBLRLRQEAAAT";
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
            press.type = SDL_KEYDOWN;
            char c = *script++;
            switch (c) {
            case 'F': press.key.keysym.sym = SDLK_UP; break;
            case 'B': press.key.keysym.sym = SDLK_DOWN; break;
            case 'L': press.key.keysym.sym = SDLK_LEFT; break;
            case 'R': press.key.keysym.sym = SDLK_RIGHT; break;
            case 'Q': press.key.keysym.sym = SDLK_LEFT; press.key.keysym.mod = KMOD_CTRL; break;
            case 'E': press.key.keysym.sym = SDLK_RIGHT; press.key.keysym.mod = KMOD_CTRL; break;
            case 'M': press.key.keysym.sym = SDLK_m; break;
            case 'T': press.key.keysym.sym = SDLK_r; break;
            case 'K': press.key.keysym.sym = SDLK_k; break;
            case 'S': press.key.keysym.sym = SDLK_s; break;
            case 'A': press.key.keysym.sym = SDLK_a; break;
            case 'D': press.key.keysym.sym = SDLK_d; break;
            case 'P': press.key.keysym.sym = SDLK_p; break;
            case '1': press.key.keysym.sym = SDLK_F1; break;
            case '2': press.key.keysym.sym = SDLK_F2; break;
            case '+': press.key.keysym.sym = SDLK_PLUS; break;
            case '-': press.key.keysym.sym = SDLK_MINUS; break;
            case '!': press.key.keysym.sym = SDLK_ESCAPE; break;
            default: press.type = SDL_FIRSTEVENT; break;
            }
            if (press.type == SDL_KEYDOWN) {
                SDL_PushEvent(&press);
            }
        } else if (script && shotPath) {
            drawScene(game, &renderer, pictures, x, y, facing, clock, overlay);
            writePng(shotPath, g_screen, palette, ViewScreenWidth, ViewScreenHeight, 2);
            if (g_combat.log[0]) {
                printf("combat: %s%s\n", g_combat.log, g_combat.active ? " (active)" : "");
            }
            printf("(%d, %d) facing %u clock %u -> %s\n", x, y, (unsigned)facing, clock, shotPath);
            break;
        }
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, script && *script ? 0 : 100)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
                if (key == SDLK_ESCAPE) {
                    if (overlay != OverlayNone) {
                        overlay = OverlayNone;
                    } else {
                        running = false;
                    }
                } else if (key == SDLK_PLUS || key == SDLK_EQUALS || key == SDLK_KP_PLUS) {
                    clock = (clock + 30) % 1440;
                } else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
                    clock = (clock + 1440 - 30) % 1440;
                } else if (overlay == OverlayNone) {
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
                    MainCommand command = mainCommandForKey(game, extended, code);
                    if (g_combat.active) {
                        if (key == SDLK_a) {
                            combatPlayerAttack();
                        }
                    } else if (command.action == MainActionMove && g_engagedType) {
                        /* combat is not part of this demo: stand still while a monster has the party */
                    } else if (command.action == MainActionMove) {
                        MovementResult move = movementApply(command.movement, facing);
                        if (move.deltaCol || move.deltaRow) {
                            int nx = x + move.deltaCol, ny = y + move.deltaRow;
                            if (movementInBounds(game, (uint16_t)nx, (uint16_t)ny)) {
                                buildWindow(game, x, y);
                                const DungeonGridCell *cell = dungeonGridCellAtWorldPos(&g_grid, nx, ny);
                                bool isDoor = cell && (cell->flags & 0x6000);
                                MovementCellOutcome outcome =
                                    movementClassifyCell(game, worldMapTileA(&g_map, (unsigned)ny, (unsigned)nx), worldMapTileB(&g_map, (unsigned)ny, (unsigned)nx), isDoor, false);
                                if (outcome == MovementCellClear || outcome == MovementCellSpecial) {
                                    x = nx;
                                    y = ny;
                                } else if (game == GameYendor2) {
                                    playEffect(6); /* the bump sound, _val33 */
                                }
                            }
                        } else {
                            facing = move.facing;
                        }
                        exploreRevealAroundPlayer(&g_save, x, y, facing, &revealed);
                        uint16_t newTrack;
                        if (musicRegionChanged(&g_lastMusicPage, x, y, game, g_worldDat, g_worldSize, &newTrack)) {
                            playMusic(newTrack);
                        }
                        buildWindow(game, x, y);
                        monstersTakeTurns(game, x, y);
                    } else if (command.action == MainActionRest) {
                        GameClock gameClock = {(uint16_t)clock, 1, 1, 1};
                        uint8_t globalSlots[24];
                        memset(globalSlots, 0, sizeof(globalSlots));
                        int pos[2] = {x, y};
                        RestOutcome rest = restParty(&g_save, &gameClock, &g_items, globalSlots, false, false, false, restMonstersTurn, pos);
                        clock = gameClock.minutes % 1440;
                        snprintf(g_combat.log, sizeof(g_combat.log), rest.refused ? "you cannot rest here" : rest.interrupted ? "rest interrupted in hour %u" : "rested 8 hours, %u fed", rest.interrupted ? rest.hour : rest.fed);
                    } else if (command.action == MainActionUnlockDoor) { /* a demo skeleton key: every tier, as chest and door key at once */
                        InteractUnlockOutcome unlock = interactUnlockFacing(&g_save, game, &g_objects, &g_locks, x, y, facing, 0xFFC0, 0);
                        static const char *const names[] = {"nothing to unlock here", "already unlocked", "unlocked", "locked (needs another key)"};
                        snprintf(g_combat.log, sizeof(g_combat.log), "%s", names[unlock.result]);
                    } else if (command.action == MainActionAct) { /* loot the chest ahead: every slot that is still there */
                        WorldObjectProbeResult probe = worldObjectProbeFacingTile(&g_objects, game, x, y, facing);
                        LockRecord lock;
                        unsigned looted = 0;
                        if (probe.outcome != WorldObjectProbeNone && (probe.object.flags & WorldObjectFlagDoor) && lockCatalogRecord(&g_locks, probe.object.value, &lock)) {
                            for (unsigned slot = 0; slot < LockContentSlots; slot++) {
                                looted += chestTake(&g_save, probe.object.value, &lock, &g_items, slot, NULL);
                            }
                        }
                        snprintf(g_combat.log, sizeof(g_combat.log), "looted %u slots", looted);
                    } else if (command.action == MainActionLocalMap) {
                        overlay = OverlayLocalMap;
                    } else if (command.action == MainActionGameDialog) {
                        overlay = OverlayDialog;
                    } else if (command.action == MainActionMemberDetail) {
                        g_overlayHero = command.index & 3;
                        overlay = OverlayDetail;
                    } else if (command.action == MainActionPartyInventory) {
                        overlay = OverlayDolls;
                    }
                }
                dirty = true;
            } else if (event.type == SDL_WINDOWEVENT) {
                dirty = true;
            }
        }
        if (dirty) {
            drawScene(game, &renderer, pictures, x, y, facing, clock, overlay);
            static uint32_t pixels[ViewScreenWidth * ViewScreenHeight];
            for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
                pixels[i] = colours[g_screen[i]];
            }
            SDL_UpdateTexture(texture, NULL, pixels, ViewScreenWidth * 4);
            SDL_RenderClear(sdl);
            SDL_RenderCopy(sdl, texture, NULL, NULL);
            SDL_RenderPresent(sdl);
            char title[256];
            snprintf(title, sizeof(title), "Yendorian Tales - (%d, %d) clock %02u:%02u%s", x, y, clock / 60, clock % 60, g_combat.active ? " - COMBAT: A attacks" : "");
            if (g_combat.log[0]) {
                size_t used = strlen(title);
                snprintf(title + used, sizeof(title) - used, " - %s", g_combat.log);
            }
            SDL_SetWindowTitle(window, title);
            dirty = false;
        }
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    pictureFileClose(pictures);
    return 0;
}

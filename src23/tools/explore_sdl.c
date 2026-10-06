/*
 * An interactive walk through a real map: the decoded modules driven by a keyboard, in an SDL2 window -- movement and passability, the fog-of-war
 * reveal, the first-person view with the day/night lighting, the minimap, the four party panels, the local area map (M) and the pause dialog (D).
 * It is the smallest "engine" that uses them together; nothing here is game logic (that is all in the modules), only input and display glue.
 *
 * Build and run (from src23/tools; SDL2 from C:\sdk\SDL2-2.32.10, SDL2.dll next to the exe or on PATH):
 *   gcc -Wall -Wextra -std=c99 -I .. -I /c/sdk/SDL2-2.32.10/include -o explore_sdl explore_sdl.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c \
 *       ../monsterpool.c ../monster.c ../monster_stdio.c ../globalflags.c ../viewrender.c ../minimap.c ../statuspanel.c ../font.c ../uiregions.c \
 *       ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c \
 *       ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../palette.c \
 *       -L /c/sdk/SDL2-2.32.10/lib -lmingw32 -lSDL2main -lSDL2
 *   ./explore_sdl <2|3> <game dir> [scale]
 *
 * Keys: Up / Down walk, Left / Right turn, Ctrl+Left / Ctrl+Right strafe (the original's scan codes, through mainCommandForKey), M the local area
 * map, D the pause dialog, + / - move the clock by 30 minutes (watch the lighting), Escape closes an overlay or quits.
 *
 * Headless check: with EXPLORE_KEYS set (F B L R forward / back / turn left / turn right, Q E strafe, M map, D dialog, + -, ESC as '!') the keys are played
 * at start and the final screen is written to the PNG named by EXPLORE_SHOT, then the program exits (SDL_VIDEODRIVER=dummy needs no display).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "dungeongrid.h"
#include "explore.h"
#include "gamedialog.h"
#include "lighting.h"
#include "localmap.h"
#include "maininput.h"
#include "minimap.h"
#include "monster_stdio.h"
#include "monsterpool.h"
#include "movement.h"
#include "newgame.h"
#include "pictures_stdio.h"
#include "pngwrite.h"
#include "statuspanel.h"
#include "viewport.h"
#include "viewrender.h"
#include "windowbake.h"
#include "worldmap_stdio.h"

typedef enum { OverlayNone, OverlayLocalMap, OverlayDialog } Overlay;

static WorldMap g_map;
static WorldObjectTable g_objects;
static LockCatalog g_locks;
static MonsterCatalog g_catalog;
static uint8_t g_pool[MonsterPoolSize * MonsterRecordSize];
static RandomState g_rng;
static int g_engagedType = 0;
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
        }
    }
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
    randomStart(&g_rng, 30, 50);
    saveGameInit(&g_save, game);
    if (!saveGameNewGame(&g_save, game, worldDat, worldSize)) {
        fprintf(stderr, "cannot build the new game\n");
        return 1;
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
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    Overlay overlay = OverlayNone;
    bool running = true, dirty = true;
    const char *script = getenv("EXPLORE_KEYS");
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
            case 'D': press.key.keysym.sym = SDLK_d; break;
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
            printf("(%d, %d) facing %u clock %u -> %s\n", x, y, (unsigned)facing, clock, shotPath);
            break;
        }
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, 100)) {
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
                    } else if (key >= SDLK_a && key <= SDLK_z) {
                        extended = false;
                        code = (uint8_t)(key - SDLK_a + 'A');
                    }
                    MainCommand command = mainCommandForKey(game, extended, code);
                    if (command.action == MainActionMove && g_engagedType) {
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
                                }
                            }
                        } else {
                            facing = move.facing;
                        }
                        exploreRevealAroundPlayer(&g_save, x, y, facing, &revealed);
                        buildWindow(game, x, y);
                        monstersTakeTurns(game, x, y);
                    } else if (command.action == MainActionLocalMap) {
                        overlay = OverlayLocalMap;
                    } else if (command.action == MainActionGameDialog) {
                        overlay = OverlayDialog;
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
            char title[96];
            snprintf(title, sizeof(title), "Yendorian Tales - (%d, %d) clock %02u:%02u%s", x, y, clock / 60, clock % 60, g_engagedType ? " - ENGAGED (combat is not in this demo)" : "");
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

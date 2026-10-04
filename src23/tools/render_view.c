/*
 * Renders the first-person view at a map position to a PNG (palette from WORLD.DAT, stored-deflate encoder, no zlib).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o render_view render_view.c ../viewrender.c ../monster.c ../monster_stdio.c ../minimap.c ../paperdoll.c ../statsheet.c ../roster.c ../charcreate.c ../gamedialog.c ../clueitem.c ../cluemonster.c ../cluetransport.c ../exedata.c ../textfield.c ../statuspanel.c ../font.c ../uiregions.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c  *       ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../monsterpanel.c ../textpanel.c ../cluebook.c ../palette.c
 *   ./render_view <2|3> <game dir> <x> <y> <N|S|E|W> <clock minutes> <out.png>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pngwrite.h"
#include "dungeongrid.h"
#include "gamedialog.h"
#include "clueitem.h"
#include "cluemonster.h"
#include "cluetransport.h"
#include "exedata.h"
#include "lighting.h"
#include "minimap.h"
#include "monster_stdio.h"
#include "charcreate.h"
#include "newgame.h"
#include "paperdoll.h"
#include "roster.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "pictures_stdio.h"
#include "viewport.h"
#include "viewrender.h"
#include "worldmap_stdio.h"

int main(int argc, char **argv) {
    if (argc < 8) {
        fprintf(stderr, "usage: %s <2|3> <game dir> <x> <y> <N|S|E|W> <clock minutes> <out.png> [scale]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    const char *dir = argv[2];
    int x = atoi(argv[3]), y = atoi(argv[4]);
    uint16_t facing = argv[5][0] == 'N' ? SaveFacingNorth : argv[5][0] == 'S' ? SaveFacingSouth : argv[5][0] == 'E' ? SaveFacingEast : SaveFacingWest;
    unsigned clock = (unsigned)atoi(argv[6]);
    unsigned scale = argc > 8 ? (unsigned)atoi(argv[8]) : 1;

    char path[512];
    static WorldMap map;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    char path0[512];
    memcpy(path0, path, sizeof(path0));
    if (!worldMapReadWorldDatFile(&map, game, path)) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    FILE *f = fopen(path, "rb");
    static uint8_t tables[ViewTablesSize], palette[PicturePaletteSize];
    if (!f || fseek(f, (long)viewTablesOffset(game), SEEK_SET) != 0 || fread(tables, 1, sizeof(tables), f) != sizeof(tables) ||
        fseek(f, (long)pictureMasterPaletteOffset(game), SEEK_SET) != 0 || fread(palette, 1, sizeof(palette), f) != sizeof(palette)) {
        fprintf(stderr, "cannot read the view tables\n");
        return 1;
    }
    fclose(f);
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir);
    PictureFile *pictures = pictureFileOpen(path, game);
    if (!pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }

    static DungeonGrid grid;
    dungeonGridBuild(&grid, game, &map, NULL, x, y);
    DungeonGridCell cells[ViewportCellCount];
    viewportBuild(&grid, facing, x, y, cells);
    viewportComputeVisibility(game, cells);

    LightingInput light = {0, 0, (uint16_t)clock, facing};
    ViewScene scene;
    memset(&scene, 0, sizeof(scene));
    bool reset;
    lightingComputeGradient(game, &light, 0, scene.gradient, &reset);
    scene.cells = cells;
    scene.facing = facing;

    static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    static MonsterCatalog monsterCatalog;
    uint8_t monster[MonsterRecordSize];
    if (getenv("RENDER_MONSTER")) { /* RENDER_MONSTER=<type id>[,<cell>]: a monster of that type in the view (default cell 43: two cells ahead) */
        unsigned type = 0, cell = 43;
        sscanf(getenv("RENDER_MONSTER"), "%u,%u", &type, &cell);
        if (monsterCatalogReadWorldDatFile(&monsterCatalog, game, path0) && monsterRecordSpawn(monster, &monsterCatalog, type) && cell < ViewportCellCount) {
            monsterRecordStartAnimation(monster, 2);
            scene.cellMonsters[cell] = monster;
        } else {
            fprintf(stderr, "cannot make monster %u\n", type);
        }
    }
    if (getenv("RENDER_HUD")) { /* the main screen's frame: category 0 picture 1 at (1, 1) */
        const uint8_t *frame = pictureFileGet(pictures, 0, 1);
        for (unsigned row = 0; frame && row < 198; row++) {
            memcpy(&screen[(row + 1) * ViewScreenWidth + 1], &frame[row * 318], 318);
        }
    }
    ViewRenderer renderer = {game, tables, pictureFileGet, pictures, screen, NULL};
    viewRender(&renderer, &scene);
    if (getenv("RENDER_HUD")) {
        for (int r = 0; r < DungeonGridSize; r++) {
            for (int c = 0; c < DungeonGridSize; c++) {
                dungeonGridCellMutable(&grid, r, c)->flags |= 0x8000; /* show the whole map as explored */
            }
        }
        MinimapTile tiles[MinimapCells];
        int16_t shades[LightingViewportCells];
        lightingViewportTable(scene.gradient, shades);
        minimapBuild(game, &grid, x, y, tiles);
        minimapDraw(&renderer, tiles, shades, facing);
        /* the four ready-made heroes of the new-game template in the party panels */
        FILE *wf = fopen(path0, "rb");
        static uint8_t worldDat[5000000];
        size_t worldSize = wf ? fread(worldDat, 1, sizeof(worldDat), wf) : 0;
        if (wf) {
            fclose(wf);
        }
        static SaveGame save;
        saveGameInit(&save, game);
        if (worldSize && saveGameNewGame(&save, game, worldDat, worldSize)) {
            for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
                statusPanelDraw(&renderer, panel, saveGamePartyRecord(&save, 5 + panel));
            }
            if (getenv("RENDER_SHEET")) { /* RENDER_SHEET=<0-3>: that hero's character sheet */
                unsigned hero = (unsigned)atoi(getenv("RENDER_SHEET")) & 3;
                static ItemCatalog sheetItems;
                uint16_t roles[5];
                for (unsigned i = 0; i < 5; i++) {
                    roles[i] = saveHeaderGetU16(&save, SaveHeaderRoleAssignments + 2 * i);
                }
                if (itemCatalogParseWorldDat(&sheetItems, game, worldDat, worldSize)) {
                    characterSheetDraw(&renderer, &sheetItems, saveGamePartyRecord(&save, 5 + hero), 6 + hero, roles, "CHARACTER CREATION");
                }
            }
            if (getenv("RENDER_DETAIL")) { /* RENDER_DETAIL=<0-3>: that hero's detail screen over the viewport */
                unsigned hero = (unsigned)atoi(getenv("RENDER_DETAIL")) & 3;
                uint16_t droles[5];
                for (unsigned i = 0; i < 5; i++) {
                    droles[i] = saveHeaderGetU16(&save, SaveHeaderRoleAssignments + 2 * i);
                }
                detailSheetDraw(&renderer, saveGamePartyRecord(&save, 5 + hero), 6 + hero, droles);
            }
            if (getenv("RENDER_PICK")) { /* RENDER_PICK=class|portrait|items|roll: the creation steps over the picture-3 backdrop */
                const uint8_t *backdrop = pictureFileGet(pictures, 0, 3);
                for (unsigned row = 0; backdrop && row < 198; row++) {
                    memcpy(&screen[(row + 1) * ViewScreenWidth + 1], &backdrop[row * 318], 318);
                }
                if (getenv("RENDER_PICK")[0] == 'c') {
                    charCreateClassPickDraw(&renderer);
                } else if (getenv("RENDER_PICK")[0] == 'r') { /* the roll screen: the sheet plus the option texts */
                    charCreateRollOptionsDraw(&renderer);
                } else if (getenv("RENDER_PICK")[0] == 'i') { /* the first eight catalog items as the pick list */
                    static ItemCatalog pickItems;
                    uint16_t pickIds[8] = {1, 2, 3, 4, 5, 6, 7, 8};
                    if (itemCatalogParseWorldDat(&pickItems, game, worldDat, worldSize)) {
                        charCreateItemListDraw(&renderer, &pickItems, pickIds, 0, false);
                    }
                } else {
                    charCreatePortraitGridDraw(&renderer, 2);
                }
            }
            if (getenv("RENDER_DIALOG")) { /* RENDER_DIALOG=<ui flags, e.g. 252>: the pause dialog over the view */
                gameDialogDraw(&renderer, (unsigned)atoi(getenv("RENDER_DIALOG")), DriverMusicAvailable | DriverSoundFxAvailable | DriverMusicOn, 5);
            }
            if (getenv("RENDER_ITEMPAGE")) { /* RENDER_ITEMPAGE=<item id>: its clue book page (labels read from the executable) */
                static ItemCatalog pageItems;
                static ClueItemText pageText;
                char exePath[512];
                snprintf(exePath, sizeof(exePath), "%s/%s", dir, game == GameYendor2 ? "SW.EXE" : "REGISTER.EXE");
                FILE *ef = fopen(exePath, "rb");
                static uint8_t exeBytes[400000];
                size_t exeSize = ef ? fread(exeBytes, 1, sizeof(exeBytes), ef) : 0;
                if (ef) {
                    fclose(ef);
                }
                ExeData exe;
                if (itemCatalogParseWorldDat(&pageItems, game, worldDat, worldSize) && exeDataOpen(&exe, game, exeBytes, exeSize) && clueItemTextLoad(&pageText, &exe, game)) {
                    const uint8_t *rec = itemCatalogRecord(&pageItems, (unsigned)atoi(getenv("RENDER_ITEMPAGE")));
                    if (rec) {
                        clueItemPageDraw(&renderer, &pageText, rec, 0x8000, 12);
                        const uint8_t *entry = itemTargetEntry(&pageItems, rec);
                        ItemTargetKind kind = itemTargetKind(rec);
                        if (entry && kind == ItemTargetWearable) {
                            clueArmorRowDraw(&renderer, &pageText, entry, itemEffectEntry(&pageItems, rec));
                        } else if (entry && kind == ItemTargetWeapon) {
                            clueWeaponRowDraw(&renderer, &pageText, entry);
                        } else if (entry && kind == ItemTargetConsumable) {
                            clueHealingRowDraw(&renderer, &pageText, game, entry);
                        }
                    }
                } else {
                    fprintf(stderr, "cannot build the item page\n");
                }
            }
            if (getenv("RENDER_MONPAGE")) { /* RENDER_MONPAGE=<monster type id>: its clue book statistics page */
                static MonsterCatalog pageMonsters;
                static ClueMonsterText monText;
                char exePath[512];
                snprintf(exePath, sizeof(exePath), "%s/%s", dir, game == GameYendor2 ? "SW.EXE" : "REGISTER.EXE");
                FILE *ef = fopen(exePath, "rb");
                static uint8_t monExe[400000];
                size_t monExeSize = ef ? fread(monExe, 1, sizeof(monExe), ef) : 0;
                if (ef) {
                    fclose(ef);
                }
                ExeData exe;
                uint8_t rec[MonsterRecordSize];
                if (monsterCatalogReadWorldDatFile(&pageMonsters, game, path0) && exeDataOpen(&exe, game, monExe, monExeSize) && clueMonsterTextLoad(&monText, &exe, game) &&
                    monsterRecordSpawn(rec, &pageMonsters, (unsigned)atoi(getenv("RENDER_MONPAGE")))) {
                    char name[MonsterNameBufferSize];
                    monsterGetName(rec, name);
                    clueMonsterPageDraw(&renderer, &monText, rec, name, 0x8000);
                } else {
                    fprintf(stderr, "cannot build the monster page\n");
                }
            }
            if (getenv("RENDER_TRANSPORT")) { /* the clue book transportation page, read from the executable */
                static ClueTransportData transport;
                char exePath[512];
                snprintf(exePath, sizeof(exePath), "%s/%s", dir, game == GameYendor2 ? "SW.EXE" : "REGISTER.EXE");
                FILE *tf = fopen(exePath, "rb");
                static uint8_t transportExe[400000];
                size_t transportSize = tf ? fread(transportExe, 1, sizeof(transportExe), tf) : 0;
                if (tf) {
                    fclose(tf);
                }
                ExeData texe;
                if (exeDataOpen(&texe, game, transportExe, transportSize) && clueTransportLoad(&transport, &texe, game)) {
                    clueTransportPageDraw(&renderer, &transport, 0x8000);
                }
            }
            if (getenv("RENDER_ROSTER")) { /* the roster screen with all nine template records */
                const uint8_t *records[RosterSlots];
                for (unsigned i = 0; i < RosterSlots; i++) {
                    records[i] = saveGamePartyRecord(&save, i);
                }
                rosterDraw(&renderer, records);
            }
            if (getenv("RENDER_DOLLS")) { /* RENDER_DOLLS=1: the four paper dolls over the viewport, as on the inventory screen */
                static ItemCatalog items;
                if (itemCatalogParseWorldDat(&items, game, worldDat, worldSize)) {
                    for (unsigned i = 0; i < 4; i++) {
                        paperDollDraw(&renderer, &items, saveGamePartyRecord(&save, 5 + i), 8 + 56 * (int)i, 8);
                    }
                }
            }
        }
    }
    pictureFileClose(pictures);
    if (!writePng(argv[7], screen, palette, ViewScreenWidth, ViewScreenHeight, scale)) {
        fprintf(stderr, "cannot write %s\n", argv[7]);
        return 1;
    }
    printf("wrote %s (gradient %d %d %d %d %d %d %d)\n", argv[7], scene.gradient[0], scene.gradient[1], scene.gradient[2], scene.gradient[3],
           scene.gradient[4], scene.gradient[5], scene.gradient[6]);
    return 0;
}

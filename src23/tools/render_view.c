/*
 * Renders the first-person view at a map position to a PNG (palette from WORLD.DAT, stored-deflate encoder, no zlib).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o render_view render_view.c ../viewrender.c ../monster.c ../monster_stdio.c ../minimap.c ../paperdoll.c ../statsheet.c ../roster.c ../statuspanel.c ../font.c ../uiregions.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c \
 *       ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c
 *   ./render_view <2|3> <game dir> <x> <y> <N|S|E|W> <clock minutes> <out.png>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dungeongrid.h"
#include "lighting.h"
#include "minimap.h"
#include "monster_stdio.h"
#include "newgame.h"
#include "paperdoll.h"
#include "roster.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "pictures_stdio.h"
#include "viewport.h"
#include "viewrender.h"
#include "worldmap_stdio.h"

static uint32_t crcTable[256];

static void crcInit(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        }
        crcTable[n] = c;
    }
}

static uint32_t crc(uint32_t c, const uint8_t *p, size_t n) {
    c = ~c;
    while (n--) {
        c = crcTable[(c ^ *p++) & 0xFF] ^ (c >> 8);
    }
    return ~c;
}

static void be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void chunk(FILE *f, const char *tag, const uint8_t *data, size_t n) {
    uint8_t head[8];
    be32(head, (uint32_t)n);
    memcpy(head + 4, tag, 4);
    fwrite(head, 1, 8, f);
    fwrite(data, 1, n, f);
    uint32_t c = crc(0, (const uint8_t *)tag, 4);
    c = crc(c, data, n);
    uint8_t tail[4];
    be32(tail, c);
    fwrite(tail, 1, 4, f);
}

static bool writePng(const char *path, const uint8_t *indexed, const uint8_t *palette, unsigned w, unsigned h, unsigned scale) {
    crcInit();
    unsigned ow = w * scale, oh = h * scale;
    size_t rowBytes = 1 + (size_t)ow * 3, rawSize = rowBytes * oh;
    uint8_t *raw = malloc(rawSize);
    for (unsigned y = 0; y < oh; y++) {
        uint8_t *row = raw + y * rowBytes;
        row[0] = 0;
        for (unsigned x = 0; x < ow; x++) {
            const uint8_t *c = &palette[indexed[(y / scale) * w + x / scale] * 3];
            for (int k = 0; k < 3; k++) {
                row[1 + x * 3 + k] = (uint8_t)(c[k] * 4 + c[k] / 16);
            }
        }
    }
    size_t blocks = (rawSize + 65534) / 65535;
    uint8_t *z = malloc(rawSize + blocks * 5 + 6);
    size_t zn = 0;
    z[zn++] = 0x78;
    z[zn++] = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < rawSize; i++) {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }
    for (size_t off = 0; off < rawSize; off += 65535) {
        size_t n = rawSize - off < 65535 ? rawSize - off : 65535;
        z[zn++] = off + n >= rawSize;
        z[zn++] = (uint8_t)n;
        z[zn++] = (uint8_t)(n >> 8);
        z[zn++] = (uint8_t)~n;
        z[zn++] = (uint8_t)(~n >> 8);
        memcpy(z + zn, raw + off, n);
        zn += n;
    }
    be32(z + zn, (b << 16) | a);
    zn += 4;
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    be32(ihdr, ow);
    be32(ihdr + 4, oh);
    ihdr[8] = 8;
    ihdr[9] = 2;
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z, zn);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);
    return true;
}

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

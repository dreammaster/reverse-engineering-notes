/*
 * Walks the new-game party through a real map with the decoded movement rules and writes the first-person view after every step
 * as a contact sheet (a headless smoke test of movement + passability + the renderer working together).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o walk walk.c ../viewrender.c ../font.c ../uiregions.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c \
 *       ../random.c ../viewport.c ../pictures.c ../pictures_stdio.c ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c
 *   ./walk <2|3> <game dir> <keys> <out.png> [columns]
 *
 * keys: F forward, B back, L turn left, R turn right, Q strafe left, E strafe right (anything else is ignored). The start is the new game's
 * position, facing and clock. Each step prints the new position, facing and what the passability decision said.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pngwrite.h"

#include "dungeongrid.h"
#include "lighting.h"
#include "movement.h"
#include "newgame.h"
#include "pictures_stdio.h"
#include "viewport.h"
#include "viewrender.h"
#include "worldmap_stdio.h"

enum { MaxFrames = 64, FrameW = 224, FrameH = 136 };

static const char *facingName(uint16_t facing) {
    return facing == SaveFacingNorth ? "N" : facing == SaveFacingEast ? "E" : facing == SaveFacingSouth ? "S" : "W";
}

static const char *outcomeName(MovementCellOutcome o) {
    switch (o) {
    case MovementCellDoor:
        return "door (blocked)";
    case MovementCellSpecial:
        return "special cell (enters)";
    case MovementCellVoid:
        return "void (blocked)";
    case MovementCellBlocked:
        return "blocked (bump)";
    default:
        return "ok";
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s <2|3> <game dir> <keys> <out.png> [columns]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    const char *dir = argv[2], *keys = argv[3];
    unsigned columns = argc > 5 ? (unsigned)atoi(argv[5]) : 4;
    if (columns == 0) {
        columns = 4;
    }

    char path[512];
    static WorldMap map;
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir);
    if (!worldMapReadWorldDatFile(&map, game, path)) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    static uint8_t worldDat[5000000], tables[ViewTablesSize], palette[PicturePaletteSize];
    FILE *f = fopen(path, "rb");
    size_t worldSize = f ? fread(worldDat, 1, sizeof(worldDat), f) : 0;
    if (f) {
        fclose(f);
    }
    if (!worldSize || viewTablesOffset(game) + sizeof(tables) > worldSize || pictureMasterPaletteOffset(game) + sizeof(palette) > worldSize) {
        fprintf(stderr, "cannot read the view tables\n");
        return 1;
    }
    memcpy(tables, worldDat + viewTablesOffset(game), sizeof(tables));
    memcpy(palette, worldDat + pictureMasterPaletteOffset(game), sizeof(palette));
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir);
    PictureFile *pictures = pictureFileOpen(path, game);
    if (!pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    static SaveGame save;
    saveGameInit(&save, game);
    if (!saveGameNewGame(&save, game, worldDat, worldSize)) {
        fprintf(stderr, "cannot build the new game\n");
        return 1;
    }
    int x = saveHeaderGetU16(&save, SaveHeaderWorldX), y = saveHeaderGetU16(&save, SaveHeaderWorldY);
    uint16_t facing = saveHeaderGetU16(&save, SaveHeaderFacing);
    unsigned clock = saveHeaderGetU16(&save, SaveHeaderClockMinutes);
    printf("start (%d, %d) facing %s, clock %u\n", x, y, facingName(facing), clock);

    unsigned frames = 1, keyCount = (unsigned)strlen(keys);
    if (keyCount + 1 > MaxFrames) {
        keyCount = MaxFrames - 1;
    }
    unsigned rows = (keyCount + 1 + columns - 1) / columns;
    unsigned sheetW = columns * FrameW, sheetH = rows * FrameH;
    uint8_t *sheet = calloc((size_t)sheetW * sheetH, 1);
    static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    ViewRenderer renderer = {game, tables, pictureFileGet, pictures, screen, NULL};
    static DungeonGrid grid;

    for (unsigned step = 0;; step++) {
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
        memset(screen, 0, sizeof(screen));
        viewRender(&renderer, &scene);
        unsigned frame = frames - 1;
        unsigned fx = (frame % columns) * FrameW, fy = (frame / columns) * FrameH;
        for (unsigned row = 0; row < FrameH; row++) {
            memcpy(&sheet[(size_t)(fy + row) * sheetW + fx], &screen[(8 + row) * ViewScreenWidth + 8], FrameW);
        }
        if (step >= keyCount) {
            break;
        }
        char key = keys[step];
        MovementAction action;
        switch (key) {
        case 'F':
            action = MovementForward;
            break;
        case 'B':
            action = MovementBackward;
            break;
        case 'L':
            action = MovementTurnLeft;
            break;
        case 'R':
            action = MovementTurnRight;
            break;
        case 'Q':
            action = MovementStrafeLeft;
            break;
        case 'E':
            action = MovementStrafeRight;
            break;
        default:
            continue;
        }
        MovementResult move = movementApply(action, facing);
        const char *what = "ok";
        if (move.deltaCol || move.deltaRow) {
            int nx = x + move.deltaCol, ny = y + move.deltaRow;
            if (!movementInBounds(game, (uint16_t)nx, (uint16_t)ny)) {
                what = "outside the playable area";
            } else {
                const DungeonGridCell *cell = dungeonGridCellAtWorldPos(&grid, nx, ny);
                bool isDoor = cell && (cell->flags & 0x6000);
                MovementCellOutcome outcome = movementClassifyCell(game, worldMapTileA(&map, (unsigned)ny, (unsigned)nx), worldMapTileB(&map, (unsigned)ny, (unsigned)nx), isDoor, false);
                what = outcomeName(outcome);
                if (outcome == MovementCellClear || outcome == MovementCellSpecial) {
                    x = nx;
                    y = ny;
                }
            }
        } else {
            facing = move.facing;
        }
        printf("%c -> (%d, %d) facing %s: %s\n", key, x, y, facingName(facing), what);
        frames++;
    }
    pictureFileClose(pictures);
    bool ok = writePng(argv[4], sheet, palette, sheetW, sheetH, 1);
    free(sheet);
    if (!ok) {
        fprintf(stderr, "cannot write %s\n", argv[4]);
        return 1;
    }
    printf("wrote %s (%u frames)\n", argv[4], frames);
    return 0;
}

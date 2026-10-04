/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_minimap test_minimap.c ../minimap.c ../viewrender.c ../random.c ../pictures.c ../dungeongrid.c ../movement.c \
 *       ../worldmap.c ../savegame.c && ./test_minimap
 */
#include <stdio.h>
#include <string.h>

#include "minimap.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static const uint8_t *tilePicture(void *ctx, unsigned category, unsigned id) {
    static uint8_t tile[64];
    (void)ctx;
    if (category != MinimapCategory) {
        return NULL;
    }
    memset(tile, (uint8_t)(id & 0x7F), sizeof(tile));
    if (id == 7) { /* an overlay picture with a transparent corner */
        tile[0] = 0xFF;
    }
    return tile;
}

int main(void) {
    static DungeonGrid grid;
    memset(&grid, 0, sizeof(grid));
    grid.game = GameYendor2;
    grid.originCol = 100;
    grid.originRow = 20;
    /* the party at (140, 60); mark the cell two columns left and one row up explored with wall type 2 and floor type 5 */
    DungeonGridCell *cell = dungeonGridCellMutable(&grid, 60 - 1 - 20, 140 - 2 - 100);
    cell->flags = 0x8000;
    cell->wallType = 2;
    cell->floorType = 5;
    MinimapTile tiles[MinimapCells];
    minimapBuild(GameYendor2, &grid, 140, 60, tiles);
    /* column = x - (partyX - 4) = 2, row = y - (partyY - 3) = 2 */
    uint16_t wallPic, floorPic;
    worldMapWallPictureOffset(GameYendor2, 2, &wallPic);
    worldMapFloorPictureOffset(GameYendor2, 5, &floorPic);
    check("an explored cell takes its legend pictures", tiles[2 * MinimapColumns + 2].base == wallPic && tiles[2 * MinimapColumns + 2].overlay == floorPic);
    check("an unexplored cell is the blank tile 0x13", tiles[0].base == MinimapBlankTile && tiles[0].overlay == 0 && tiles[31].base == MinimapBlankTile);

    check("the compass arrow per facing", minimapCompassPicture(SaveFacingNorth) == 0 && minimapCompassPicture(SaveFacingSouth) == 2 &&
                                              minimapCompassPicture(SaveFacingEast) == 1 && minimapCompassPicture(SaveFacingWest) == 3);

    static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    ViewRenderer r = {GameYendor2, NULL, tilePicture, NULL, screen, NULL};
    MinimapTile simple[MinimapCells];
    memset(simple, 0, sizeof(simple));
    for (unsigned i = 0; i < MinimapCells; i++) {
        simple[i].base = 0x30;
    }
    simple[0].base = 0x21;
    simple[1].base = 0x22;
    simple[1].overlay = 7;
    int16_t shades[MinimapCells];
    memset(shades, 0, sizeof(shades));
    shades[0] = -1;
    memset(screen, 0, sizeof(screen));
    minimapDraw(&r, simple, shades, SaveFacingNorth);
    check("tiles are 8 x 8 at (240, 8) and shaded by the cell's delta", screen[8 * 320 + 240] == viewShadeColour(0x21, -1) && screen[15 * 320 + 247] == viewShadeColour(0x21, -1));
    check("the next tile starts 8 pixels on", screen[8 * 320 + 248] == 0x22 && screen[8 * 320 + 250] == 7);
    check("an overlay is drawn over its base, 0xFF skipped", screen[8 * 320 + 248] == 0x22 && screen[9 * 320 + 249] == 7);
    check("the second row starts 8 pixels down", screen[16 * 320 + 240] == 0x30);
    check("the compass arrow is drawn over the grid at (272, 32), unshaded", screen[32 * 320 + 272] == 0 && screen[31 * 320 + 272] == 0x30);
    memset(shades, 0xFF, sizeof(shades)); /* all -1 */
    minimapDraw(&r, simple, shades, SaveFacingSouth);
    check("...picture 2 for south, still unshaded", screen[32 * 320 + 272] == 2 && screen[31 * 320 + 272] == viewShadeColour(0x30, -1));

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_localmap test_localmap.c ../localmap.c ../minimap.c ../explore.c ../savegame.c ../worldmap.c ../viewrender.c ../random.c ../pictures.c ../font.c ../uiregions.c ../dungeongrid.c ../movement.c ../location.c && ./test_localmap
 */
#include <stdio.h>
#include <string.h>

#include "localmap.h"
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

static const uint8_t *pic(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[64];
    (void)ctx;
    memset(pixels, (uint8_t)(category * 16 + id), sizeof(pixels));
    return pixels;
}

int main(void) {
    int c0, r0;
    localMapBlockOrigin(166, 36, &c0, &r0);
    check("block origin: (166, 36) is in the block starting at column 160, row 24", c0 == 160 && r0 == 24);
    localMapBlockOrigin(460, 46, &c0, &r0);
    check("(460, 46) -> column 440, row 24", c0 == 440 && r0 == 24);
    check("block numbers run along rows of 20", localMapBlockNumber(0, 0) == 0 && localMapBlockNumber(39, 23) == 0 && localMapBlockNumber(40, 0) == 1 &&
                                                   localMapBlockNumber(0, 24) == 20 && localMapBlockNumber(166, 36) == 24 && localMapBlockNumber(799, 167) == 6 * 20 + 19);

    uint16_t wall = 0, floorType = 0, found;
    for (uint16_t t = 0; t < 60 && !wall; t++) {
        if (worldMapWallPictureOffset(GameYendor2, t, &found) && found != MinimapBlankTile && found != 0) {
            wall = t;
        }
    }
    for (uint16_t t = 0; t < 70 && !floorType; t++) {
        if (worldMapFloorPictureOffset(GameYendor2, t, &found) && found != 0) {
            floorType = t;
        }
    }
    check("the test found a wall and a floor type with local map pictures", wall != 0 && floorType != 0);
    uint16_t wallPicture = 0, floorPicture = 0;
    worldMapWallPictureOffset(GameYendor2, wall, &wallPicture);
    worldMapFloorPictureOffset(GameYendor2, floorType, &floorPicture);

    static LocalMapCell cells[LocalMapColumns * LocalMapRows];
    memset(cells, 0, sizeof(cells));
    cells[0].explored = true;
    cells[0].wallType = wall;
    cells[1].explored = true;
    cells[1].wallType = wall;
    cells[1].floorType = floorType;
    cells[LocalMapColumns * LocalMapRows - 1].explored = true;
    cells[LocalMapColumns * LocalMapRows - 1].wallType = wall;
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, pic, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    localMapDraw(&r, cells, 5, 5, 0);
    check("an explored cell shows its wall picture at (0, 8)", screen[8 * 320] == 9 * 16 + wallPicture);
    check("an unexplored cell is the blank tile 0x13", screen[8 * 320 + 16] == 9 * 16 + MinimapBlankTile);
    check("the floor picture overlays the wall of cell 1 at (8, 8) -- or leaves it if transparent", screen[8 * 320 + 8] == 9 * 16 + floorPicture || screen[8 * 320 + 8] == 9 * 16 + wallPicture);
    check("the last cell is at (312, 192) and nothing is drawn above y = 8 or below y = 200", screen[192 * 320 + 312] == 9 * 16 + wallPicture && screen[7 * 320] == 0xEE &&
                                                                                                 screen[199 * 320 + 319] == 9 * 16 + wallPicture);
    memset(screen, 0xEE, sizeof(screen));
    localMapDraw(&r, cells, 45, 26, SaveFacingNorth);
    check("the arrow (picture 0) is at (8 * (45 mod 40), 8 * (26 mod 24 + 1)) = (40, 24)", screen[24 * 320 + 40] == 9 * 16 + 0);
    memset(screen, 0xEE, sizeof(screen));
    localMapDraw(&r, cells, 45, 26, SaveFacingSouth);
    check("south faces with picture 2", screen[24 * 320 + 40] == 9 * 16 + 2);

    int mx, my;
    check("the overview marker: (166, 36) is above the pictured area, (166, 60) is inside", !overviewMapMarker(166, 36, &mx, &my) && overviewMapMarker(166, 60, &mx, &my) && mx == 28 && my == 23 + 20 * 0);
    check("...(460, 100) -> block 7 across, 2 down", overviewMapMarker(460, 100, &mx, &my) && mx == 28 + 24 * 7 && my == 23 + 20 * 2);
    check("...and the area ends at column 639 / row 239", overviewMapMarker(639, 239, &mx, &my) && mx == 28 + 24 * 11 && my == 23 + 20 * 7 && !overviewMapMarker(640, 100, &mx, &my) &&
                                                          !overviewMapMarker(300, 240, &mx, &my) && !overviewMapMarker(159, 100, &mx, &my));
    LocationName where = {"PORT", " MAP 1", 1};
    memset(screen, 0xEE, sizeof(screen));
    localMapHeaderDraw(&r, &where);
    bool nameOk = false, suffixOk = false;
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 24; x++) {
            nameOk = nameOk || screen[y * 320 + x] == 0x8A;
        }
        for (int x = 30; x < 70; x++) {
            suffixOk = suffixOk || screen[y * 320 + x] == 0x5B;
        }
    }
    check("the header: the name in 0x8A at x = 0, a map suffix in 0x5B after name length + 1 characters", nameOk && suffixOk);
    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

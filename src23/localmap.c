#include "localmap.h"

#include "explore.h"
#include "font.h"
#include <string.h>
#include "minimap.h"
#include "pictures.h"

void localMapBlockOrigin(int x, int y, int *firstColumn, int *firstRow) {
    *firstColumn = x - x % LocalMapColumns;
    *firstRow = y - y % LocalMapRows;
}

unsigned localMapBlockNumber(int x, int y) {
    return (unsigned)(y / LocalMapRows) * LocalMapBlocksPerRow + (unsigned)(x / LocalMapColumns);
}

void localMapFill(LocalMapCell cells[LocalMapColumns * LocalMapRows], GameKind game, const WorldMap *map, SaveGame *save, int firstColumn, int firstRow) {
    (void)game;
    for (int row = 0; row < LocalMapRows; row++) {
        for (int column = 0; column < LocalMapColumns; column++) {
            LocalMapCell *cell = &cells[row * LocalMapColumns + column];
            int x = firstColumn + column, y = firstRow + row;
            bool inside = x >= 0 && x < WorldMapColumns && y >= 0 && y < (int)map->rowCount;
            cell->wallType = inside ? worldMapTileA(map, (unsigned)y, (unsigned)x) : 0;
            cell->floorType = inside ? worldMapTileB(map, (unsigned)y, (unsigned)x) : 0;
            cell->explored = inside && exploreIsExplored(save, x, y);
        }
    }
}

void localMapDraw(const ViewRenderer *r, const LocalMapCell cells[LocalMapColumns * LocalMapRows], int partyX, int partyY, uint16_t facing) {
    for (int row = 0; row < LocalMapRows; row++) {
        for (int column = 0; column < LocalMapColumns; column++) {
            const LocalMapCell *cell = &cells[row * LocalMapColumns + column];
            int x = column * LocalMapTileSize, y = LocalMapTop + row * LocalMapTileSize;
            uint16_t picture = MinimapBlankTile, overlay = 0;
            if (cell->explored) {
                uint16_t found;
                if (worldMapWallPictureOffset(r->game, cell->wallType, &found)) {
                    picture = found;
                }
                if (worldMapFloorPictureOffset(r->game, cell->floorType, &found)) {
                    overlay = found;
                }
            }
            viewDrawPicture(r, LocalMapCategory, picture, x, y, false, 0);
            if (overlay != 0) {
                viewDrawPicture(r, LocalMapCategory, overlay, x, y, true, 0);
            }
        }
    }
    if (facing) {
        int x = (partyX % LocalMapColumns) * LocalMapTileSize, y = ((partyY % LocalMapRows) + 1) * LocalMapTileSize;
        viewDrawPicture(r, LocalMapCategory, minimapCompassPicture(facing), x, y, true, 0);
    }
}

bool overviewMapMarker(int partyX, int partyY, int *markerX, int *markerY) {
    if (partyX < 160 || partyY < 48 || partyX > 639 || partyY > 239) {
        return false;
    }
    *markerX = 28 + 24 * ((partyX - 160) / 40);
    *markerY = 23 + 20 * ((partyY - 48) / 24);
    return true;
}

void overviewMapDraw(const ViewRenderer *r, int partyX, int partyY) {
    viewDrawPicture(r, 0, 6, 0, 0, false, 0);
    int x, y;
    if (overviewMapMarker(partyX, partyY, &x, &y)) {
        viewDrawPicture(r, 8, 0x11, x, y, true, 0);
    }
}

void localMapHeaderDraw(const ViewRenderer *r, const LocationName *name) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 0, 0, name->name, 0x8A, 0, FontOpaque);
    if (name->kind != 0) {
        int x = ((int)strlen(name->name) + 1) * 6;
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, 0, name->suffix, name->kind == 1 ? 0x5B : 0xAA, 0, FontOpaque);
    }
}

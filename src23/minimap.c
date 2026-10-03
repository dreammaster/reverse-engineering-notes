#include "minimap.h"

#include "pictures.h"
#include "worldmap.h"

void minimapBuild(GameKind game, const DungeonGrid *grid, int partyX, int partyY, MinimapTile out[MinimapCells]) {
    for (int row = 0; row < MinimapRows; row++) {
        for (int col = 0; col < MinimapColumns; col++) {
            MinimapTile *tile = &out[row * MinimapColumns + col];
            tile->base = MinimapBlankTile;
            tile->overlay = 0;
            const DungeonGridCell *cell = dungeonGridCellAtWorldPos(grid, partyX - 4 + col, partyY - 3 + row);
            if (!cell || !dungeonGridCellIsExplored(cell)) {
                continue;
            }
            uint16_t picture;
            if (worldMapWallPictureOffset(game, cell->wallType, &picture)) {
                tile->base = picture;
            }
            if (worldMapFloorPictureOffset(game, cell->floorType, &picture)) {
                tile->overlay = picture;
            }
        }
    }
}

unsigned minimapCompassPicture(uint16_t facing) {
    if (facing & 0x8000) {
        return 0;
    }
    if (facing & 0x4000) {
        return 2;
    }
    return (facing & 0x1000) ? 1 : 3;
}

void minimapDraw(const ViewRenderer *r, const MinimapTile tiles[MinimapCells], const int16_t *shades, uint16_t facing) {
    for (int row = 0; row < MinimapRows; row++) {
        for (int col = 0; col < MinimapColumns; col++) {
            unsigned i = row * MinimapColumns + col;
            int x = MinimapX + col * MinimapTileSize, y = MinimapY + row * MinimapTileSize;
            int8_t shade = shades ? (int8_t)shades[i] : 0;
            viewDrawPicture(r, MinimapCategory, tiles[i].base, x, y, false, shade);
            if (tiles[i].overlay != 0) {
                viewDrawPicture(r, MinimapCategory, tiles[i].overlay, x, y, true, shade);
            }
        }
    }
    viewDrawPicture(r, MinimapCategory, minimapCompassPicture(facing), MinimapCompassX, MinimapCompassY, true, 0);
}

#ifndef YENDOR23_MINIMAP_H
#define YENDOR23_MINIMAP_H

#include <stdbool.h>
#include <stdint.h>

#include "dungeongrid.h"
#include "game.h"
#include "viewrender.h"

/*
 * The minimap on the main screen: BuildMinimapTileData (yendor2.asm:30709) and DrawMinimap (:30644), identical in both games.
 * A 9-column x 7-row grid of 8 x 8 tiles (category 9 pictures of PICTURES.VGA) centred on the party (4 columns left, 3 rows up),
 * drawn at (240, 8). An explored cell (flag 0x8000) contributes its wall type's minimap picture (legend word 5) and, when
 * its floor type's minimap picture (floor legend word 4) is nonzero, an overlay on top (colour 0xFF transparent); an
 * unexplored cell is the blank tile 0x13. Both are shaded by the cell's entry of the 63-entry lighting table
 * (lighting.h: the party's own cell is entry 31). The facing arrow at (272, 32) is picture 0 (north), 2 (south), 1 (east)
 * or 3 (west), drawn transparent and unshaded. The minimap exists only while the map tiers allow it (mapview.h).
 */
enum { MinimapColumns = 9, MinimapRows = 7, MinimapCells = 63, MinimapTileSize = 8, MinimapX = 240, MinimapY = 8, MinimapBlankTile = 0x13,
       MinimapCompassX = 272, MinimapCompassY = 32, MinimapCategory = 9 };

typedef struct {
    uint16_t base;    /* tile picture */
    uint16_t overlay; /* 0 = none */
} MinimapTile;

void minimapBuild(GameKind game, const DungeonGrid *grid, int partyX, int partyY, MinimapTile out[MinimapCells]);

/* The compass arrow's picture id for a SaveFacing. */
unsigned minimapCompassPicture(uint16_t facing);

/* Draws the grid and the arrow into the renderer's screen; shades has MinimapCells entries (or NULL for none). */
void minimapDraw(const ViewRenderer *r, const MinimapTile tiles[MinimapCells], const int16_t *shades, uint16_t facing);

#endif

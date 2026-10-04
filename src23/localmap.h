#ifndef YENDOR23_LOCALMAP_H
#define YENDOR23_LOCALMAP_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "location.h"
#include "savegame.h"
#include "viewrender.h"
#include "worldmap.h"

/*
 * The local area map (ShowLocalAreaMap yendor2.asm:31676, the M key; Chapter 3 the same) and the clue book's map pages (DrawClueBookMapGrid
 * :6532): a full-screen view of one block of the world, 40 columns by 24 rows of 8 x 8 tiles, drawn from y = 8 (the first row of the
 * screen holds the block's name). The world is cut into blocks of 40 x 24 cells, 20 blocks to a row; the party's block starts at
 * (x - x mod 40, y - y mod 24) and has the (row-major, 0-based) number (y / 24) * 20 + x / 40, which the area-name text is looked up by
 * (BuildClueLocationSuffix; see cluebook.h). Each tile is a category 9 picture, the same ones as the minimap (minimap.h): a cell the party has
 * not explored is the blank tile 0x13; an explored one is its wall type's picture (wall legend word 5) with its floor type's picture (floor
 * legend word 4) over it when that is not zero. The party is marked with the compass arrow (minimapCompassPicture) at
 * (8 * (x mod 40), 8 * (y mod 24 + 1)).
 *
 * Not reproduced: while the local map is up the original also asks TryInteractAtPosition about every explored cell and, for a door or
 * trigger whose state changed (outcomes 6 and 7), draws the replacement tile it reports instead of the wall or floor.
 */
enum { LocalMapColumns = 40, LocalMapRows = 24, LocalMapBlocksPerRow = 20, LocalMapTileSize = 8, LocalMapTop = 8, LocalMapCategory = 9 };

typedef struct {
    uint16_t wallType, floorType;
    bool explored;
} LocalMapCell;

/* The first column / row of the block that contains (x, y). */
void localMapBlockOrigin(int x, int y, int *firstColumn, int *firstRow);

/* The block's number, which selects its name. */
unsigned localMapBlockNumber(int x, int y);

/* Reads the 40 x 24 cells starting at (firstColumn, firstRow): types from the world map, the explored flag from the save's bitmap. */
void localMapFill(LocalMapCell cells[LocalMapColumns * LocalMapRows], GameKind game, const WorldMap *map, SaveGame *save, int firstColumn, int firstRow);

/* The tiles, then (when `facing` is nonzero) the party arrow at the block-relative position of (partyX, partyY). */
void localMapDraw(const ViewRenderer *r, const LocalMapCell cells[LocalMapColumns * LocalMapRows], int partyX, int partyY, uint16_t facing);

/* The first line of the screen: the block's name at (0, 0) in 0x8A on 0, then its suffix one character space further in 0x5B (a map) or 0xAA (a level). */
void localMapHeaderDraw(const ViewRenderer *r, const LocationName *name);

/*
 * The overview map (ToggleMapViewMode, the W key; Chapter 2 only, yendor2.asm:32010): PICTURES category 0 picture 6 full screen at (0, 0) with
 * the "you are here" marker (DrawPlayerPositionMarker, :32051), category 8 picture 0x11 transparent, when the party is inside the
 * pictured area -- columns 160-639 and rows 48-239, one picture block of 24 x 20 pixels per 40 x 24 cells. The marker's position is
 * (28 + 24 * ((x - 160) / 40), 23 + 20 * ((y - 48) / 24)). Returns false (nothing drawn but the picture) outside it.
 */
bool overviewMapMarker(int partyX, int partyY, int *markerX, int *markerY);
void overviewMapDraw(const ViewRenderer *r, int partyX, int partyY);

#endif

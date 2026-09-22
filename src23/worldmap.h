#ifndef YENDOR23_WORLDMAP_H
#define YENDOR23_WORLDMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The game world: a single continuous tile grid at the very start of
 * WORLD.DAT (byte offset 0), not per-level files. There is no "current
 * level" selector in the loader (RefreshDungeonMapWindow, yendor2.asm:29286)
 * -- world X/Y coordinates (g_partyWorldX/Y) address this one grid directly,
 * so towns, wilderness and dungeon interiors are all baked into one
 * coordinate space.
 *
 * Row *r* lives at file offset `r * WorldMapRowSize` (row size confirmed via
 * InitGlobals' `_blockSize3 = 0x320`, record size `4*_blockSize3` = 3200
 * bytes, read by PrepareWorldDatRead with a static zero base offset).
 * WorldMapColumns (800) comes directly from the row byte size (3200 / 4
 * bytes/column). Each column is 4 bytes: two u16 tile-type indices,
 * identical to the first 4 bytes of the runtime 8-byte dungeon-grid cell
 * (see file-formats.md's "In-memory dungeon map grid").
 *
 * Row count was found from where this pattern gives way to the item
 * catalog (759/631 x 58-byte records at a different, already-decoded
 * offset) and cross-checked two independent ways: it lands on a whole
 * number of rows with no partial row, and it matches CURGAME's own
 * fog-of-war bitmap row count exactly (100 bytes/row = 800 columns / 8
 * explored-bits-per-byte; see savegame.h's SaveSectionExploredMap).
 * Both games' full row x column grid was checked against their real
 * WORLD.DAT files: every row decodes to bounded tile-index values (no
 * value large enough to be accidentally reading past the map into
 * unrelated data), and the very first rows of both games are a
 * distinctive placeholder pattern -- wall type (tileA) alternating 0/1
 * column by column, floor type (tileB) always 0 -- consistent with a
 * bordering "void" band around the real playable area rather than a
 * decode error. The last few rows are a similarly low-variety, bounded
 * pattern (different small wall-type values), not yet characterized in
 * as much detail.
 */

enum {
    WorldMapColumns = 800,
    WorldMapColumnSize = 4, /* bytes per column: two u16 tile-type indices */
    WorldMapRowSize = WorldMapColumns * WorldMapColumnSize, /* 3200 */

    WorldMapRowsYendor2 = 144,
    WorldMapRowsYendor3 = 168,
    WorldMapRowsMax = WorldMapRowsYendor3,

    /*
     * The tile-type legend tables (WorldMapWallType.../WorldMapFloorType...
     * below) are decoded for Chapter 2 only -- Chapter 3's equivalents
     * (sub_1BC98/sub_1BCDB in yendor3.asm) page their table through EMS in
     * 100-entry blocks instead of a flat array, and that paging scheme
     * isn't traced yet. worldMapWallPictureOffset/worldMapFloorPictureOffset
     * return false for GameYendor3 until it is.
     */
    WorldMapWallTypeCountYendor2 = 58,  /* real entries; the table has more reserved zero slots, unused by real data */
    WorldMapFloorTypeCountYendor2 = 68  /* real entries 0-64; 65-67 are legitimately zero (seen in real map data) */
};

typedef struct {
    uint32_t offset; /* WORLD.DAT byte offset of row 0; 0 in both games */
    uint16_t rowCount;
} WorldMapLayout;

typedef struct {
    GameKind game;
    uint16_t rowCount;
    uint8_t rows[WorldMapRowsMax * WorldMapRowSize];
} WorldMap;

const WorldMapLayout *worldMapLayout(GameKind game);

/* Parses rowCount*WorldMapRowSize bytes starting at region[0]; false if size is too small. */
bool worldMapParse(WorldMap *map, GameKind game, const uint8_t *region, size_t size);

/* Same, from a whole WORLD.DAT image already in memory. */
bool worldMapParseWorldDat(WorldMap *map, GameKind game, const uint8_t *worldDat, size_t size);

/* The 4 raw column bytes at (row, col), or NULL if out of range. */
const uint8_t *worldMapCell(const WorldMap *map, unsigned row, unsigned col);

/* The two tile-type indices of a cell; 0 (a valid "void/unused" index in both tables) if out of range. */
uint16_t worldMapTileA(const WorldMap *map, unsigned row, unsigned col); /* wall/base type -> WorldMapWallType table */
uint16_t worldMapTileB(const WorldMap *map, unsigned row, unsigned col); /* floor/overlay type -> WorldMapFloorType table */

/*
 * Picture offsets for a tile-type index (add to a category's g_pictureDir
 * base to get the actual picture to draw -- see PICTURES.VGA in
 * file-formats.md, not yet reimplemented). False if game is GameYendor3
 * (not decoded yet) or the index is out of range.
 */
bool worldMapWallPictureOffset(GameKind game, uint16_t wallType, uint16_t *outOffset);
bool worldMapFloorPictureOffset(GameKind game, uint16_t floorType, uint16_t *outOffset);

#endif

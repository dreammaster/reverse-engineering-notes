#ifndef YENDOR23_DUNGEONGRID_H
#define YENDOR23_DUNGEONGRID_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "savegame.h"
#include "worldmap.h"

/*
 * The in-memory dungeon map-grid window: a 78x78 cell region of the world
 * map (see worldmap.h), reloaded and recentered on the party's position by
 * RefreshDungeonMapWindow (yendor2.asm:29286, yendor3.asm:28425) every time
 * the party moves. Confirmed identical between both games (same windowing
 * formula, same 78-cell size, same 8-byte cell layout, same row stride
 * 0x270 = 78*8) -- only the playable bounding box the window is clamped to
 * differs, and that's already captured by movement.h's movementBounds.
 *
 * Address computation cross-checked against GetMapCellPtr
 * (yendor2.asm:11715): a cell's offset from the grid segment's base is
 * (row-originRow)*0x270 + (col-originCol)*8.
 *
 * This module covers the base window -- windowing/clamping and the raw
 * per-cell copy from the world map plus the explored-map bitmap. It does
 * NOT cover the two things RefreshDungeonMapWindow does after that base
 * copy, both out of scope for this pass:
 *   - TryInteractAtPosition's per-cell marker baking, which is what
 *     actually sets the door/lock flag (bit 0x6000, consumed by
 *     movement.h's MovementCellOutcome via a plain isDoor bool -- this
 *     module doesn't produce it) and other item/trap/trigger markers.
 *   - g_levelMonsters placement/despawn as monsters scroll into/out of the
 *     window.
 */

enum {
    DungeonGridSize = 78 /* both rows and columns */
};

typedef struct {
    uint16_t wallType;  /* +0: copied verbatim from worldMapTileA */
    uint16_t floorType; /* +2: copied verbatim from worldMapTileB */
    /*
     * +4: zeroed by the base window copy (RefreshDungeonMapWindow) and
     * separately zeroed again by HandleMovementInput's monster-despawn
     * branch when a cell's monster scrolls out of the window -- plausibly
     * an occupant index into g_levelMonsters, but the write side (monster
     * placement) isn't traced, so not modeled as anything but a raw word.
     */
    uint16_t reserved4;
    uint16_t flags; /* +6: only bit 0x8000 (explored) is set by this module; see dungeonGridCellIsExplored */
} DungeonGridCell;

typedef struct {
    GameKind game;
    int originCol, originRow; /* world coordinates of cells[0][0] */
    DungeonGridCell cells[DungeonGridSize][DungeonGridSize];
} DungeonGrid;

/*
 * The window is nominally 78x78 centered on the party (offset 39 = 78/2 in
 * each direction) but clamped so it can extend at most 15 cells past the
 * playable bounding box's edge (movementBounds), never further:
 *   origin = clamp(partyPos - 39, boundsMin - 15, boundsMax - 15)
 * matching RefreshDungeonMapWindow's origin computation exactly, for both
 * axes independently.
 */
void dungeonGridComputeOrigin(GameKind game, int partyWorldX, int partyWorldY, int *outOriginCol, int *outOriginRow);

/*
 * Builds a fresh window from map (see worldmap.h) and, if save is non-NULL,
 * the save's SaveSectionExploredMap bitmap (bit convention: MSB-first
 * within each byte, i.e. bit (7 - col%8) of byte col/8 is column col's
 * explored flag). A cell whose world row/column falls outside map's or the
 * explored bitmap's range (possible in the up-to-15-cell clamp slack past
 * the map edge) reads as wallType/floorType 0 and unexplored, the same
 * safe-default convention worldMapTileA/B already use for out-of-range
 * reads.
 */
void dungeonGridBuild(DungeonGrid *grid, GameKind game, const WorldMap *map, SaveGame *save, int partyWorldX,
                       int partyWorldY);

/* The cell at grid-local (row, col), or NULL if either is outside [0, DungeonGridSize). */
const DungeonGridCell *dungeonGridCell(const DungeonGrid *grid, int row, int col);
DungeonGridCell *dungeonGridCellMutable(DungeonGrid *grid, int row, int col);

/* Same, addressed by absolute world coordinates instead of grid-local ones; NULL if outside the current window. */
const DungeonGridCell *dungeonGridCellAtWorldPos(const DungeonGrid *grid, int worldCol, int worldRow);

bool dungeonGridCellIsExplored(const DungeonGridCell *cell); /* flags bit 0x8000 */

/*
 * +6 bit 0x400: "there's an overlay icon here", baked in by two different
 * producers, both outside this module's own build pass -- monsterpool.c's
 * scroll-relink (+4 = the live monster's type id) and TryInteractAtPosition's
 * curgame-record branch (+4 = a worldobjects.c 0x4000 record's value; see
 * file-formats.md's "In-memory dungeon map grid" section for the full
 * interaction-marker picture, not otherwise modeled here since it's
 * rendering-only past this one flag bit).
 */
enum { DungeonGridCellFlagOverlay = 0x0400 };

#endif

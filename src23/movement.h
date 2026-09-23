#ifndef YENDOR23_MOVEMENT_H
#define YENDOR23_MOVEMENT_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "savegame.h" /* SaveFacing */

/*
 * Party movement and cell passability, decoded from HandleMovementInput
 * (yendor2.asm:2004, yendor3.asm:5638) -- the shared handler for all 6
 * movement/turn actions, reached both from the arrow keys (BIOS scan codes
 * remapped to 'H'/'P'/'K'/'M') and from an on-screen 6-button directional
 * pad (a mouse hit-test dispatcher right before it maps click zones 1-6 to
 * the same 'K'/'H'/'M'/'s'/'P'/'t' key codes HandleMovementInput switches
 * on -- confirming there are exactly 6 actions, not an open-ended set).
 *
 * Both games share the exact same direction/facing math (verified
 * instruction-for-instruction identical bit/delta logic in both games'
 * disassembly); only the passability thresholds and playable bounding box
 * differ. Neither game's thresholds are a simple low/high split -- see
 * movementClassifyFloorType and movementIsFloorTypeImpassable for the
 * exact bands.
 */

typedef enum {
    MovementForward,
    MovementBackward,
    MovementTurnLeft,  /* counterclockwise: N->W->S->E->N */
    MovementTurnRight, /* clockwise: N->E->S->W->N */
    MovementStrafeLeft,
    MovementStrafeRight
} MovementAction;

typedef struct {
    uint16_t facing;  /* the new SaveFacing value (unchanged from input for a move action) */
    int8_t deltaCol;  /* world X change, -1/0/+1 */
    int8_t deltaRow;  /* world Y change, -1/0/+1 */
} MovementResult;

/*
 * Turning changes only facing (deltaCol/Row are 0); moving/strafing changes
 * only position (facing is returned unchanged). currentFacing must be
 * exactly one of the four SaveFacing bits.
 */
MovementResult movementApply(MovementAction action, uint16_t currentFacing);

/*
 * The playable bounding box within the world map's full grid (see
 * worldmap.h) -- movement outside it is always rejected, before any tile
 * lookup. This is a strict subset of the full map: the map's own bordering
 * "void" rows/columns (wall type alternating 0/1) sit outside it.
 */
typedef struct {
    uint16_t colMin, colMax; /* inclusive */
    uint16_t rowMin, rowMax; /* inclusive */
} MovementBounds;

const MovementBounds *movementBounds(GameKind game);
bool movementInBounds(GameKind game, uint16_t col, uint16_t row);

/* ClassifyFloorType's 3-way result, operating on a cell's wall type (worldMapTileA). */
typedef enum {
    MovementFloorNormal,  /* definitely walkable, no further check needed */
    MovementFloorBlocked, /* blocked; plays a "bump" sound */
    MovementFloorVoid     /* blocked silently, no sound -- the map's own border band */
} MovementFloorClass;

MovementFloorClass movementClassifyFloorType(GameKind game, uint16_t wallType);

/* IsCellTypeImpassable, operating on a cell's floor/overlay type (worldMapTileB). */
bool movementIsFloorTypeImpassable(GameKind game, uint16_t floorType);

/*
 * The narrow wall-type range that always triggers HandleSpecialCellEntry
 * (not reimplemented here) instead of an ordinary passability check --
 * Chapter 2: 6-11; Chapter 3: 200-299 (its whole "page 2").
 */
bool movementIsSpecialWallType(GameKind game, uint16_t wallType);

typedef enum {
    MovementCellDoor,    /* the runtime grid cell's own flags mark it a door/lock; always blocks, triggers ShowLockStatus */
    MovementCellSpecial, /* movementIsSpecialWallType; always enters, triggers HandleSpecialCellEntry */
    MovementCellVoid,    /* MovementFloorVoid; blocked silently */
    MovementCellBlocked, /* blocked; plays the "bump" sound */
    MovementCellClear    /* movement is allowed */
} MovementCellOutcome;

/*
 * The full per-cell decision HandleMovementInput makes for the cell the
 * party is about to step into, in the same order the original checks them:
 * door, then special wall type, then (unless forceMove) ordinary floor
 * passability. isDoor is the runtime grid cell's +6 flags word bit 0x6000
 * (not modeled here -- a future dungeon-grid module's own concern);
 * forceMove is g_uiScratchFlags1 bit 0x8000, an override seen set before
 * calling HandleMovementInput in contexts not yet traced.
 */
MovementCellOutcome movementClassifyCell(GameKind game, uint16_t wallType, uint16_t floorType, bool isDoor,
                                          bool forceMove);

#endif

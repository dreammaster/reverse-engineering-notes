#include "movement.h"

#include <stddef.h>

/*
 * HandleMovementInput turns a facing by remapping the SaveFacing bit
 * directly (there's no numeric direction index in the original -- it's a
 * 4-entry jump/compare chain on the bit itself), and moves by adding a
 * facing-dependent (deltaCol, deltaRow) pair to the party's world position.
 * Strafing moves perpendicular to facing; forward/backward move along it.
 * Traced identically in both yendor2.asm:2004 and yendor3.asm:5638.
 */
MovementResult movementApply(MovementAction action, uint16_t currentFacing) {
    MovementResult result;
    result.facing = currentFacing;
    result.deltaCol = 0;
    result.deltaRow = 0;

    switch (action) {
        case MovementTurnLeft:
            switch (currentFacing) {
                case SaveFacingNorth: result.facing = SaveFacingWest; break;
                case SaveFacingWest: result.facing = SaveFacingSouth; break;
                case SaveFacingSouth: result.facing = SaveFacingEast; break;
                case SaveFacingEast: result.facing = SaveFacingNorth; break;
            }
            return result;

        case MovementTurnRight:
            switch (currentFacing) {
                case SaveFacingNorth: result.facing = SaveFacingEast; break;
                case SaveFacingEast: result.facing = SaveFacingSouth; break;
                case SaveFacingSouth: result.facing = SaveFacingWest; break;
                case SaveFacingWest: result.facing = SaveFacingNorth; break;
            }
            return result;

        case MovementForward:
            switch (currentFacing) {
                case SaveFacingNorth: result.deltaRow = -1; break;
                case SaveFacingSouth: result.deltaRow = 1; break;
                case SaveFacingEast: result.deltaCol = 1; break;
                case SaveFacingWest: result.deltaCol = -1; break;
            }
            return result;

        case MovementBackward:
            switch (currentFacing) {
                case SaveFacingNorth: result.deltaRow = 1; break;
                case SaveFacingSouth: result.deltaRow = -1; break;
                case SaveFacingEast: result.deltaCol = -1; break;
                case SaveFacingWest: result.deltaCol = 1; break;
            }
            return result;

        case MovementStrafeLeft:
            switch (currentFacing) {
                case SaveFacingNorth: result.deltaCol = -1; break;
                case SaveFacingSouth: result.deltaCol = 1; break;
                case SaveFacingEast: result.deltaRow = -1; break;
                case SaveFacingWest: result.deltaRow = 1; break;
            }
            return result;

        case MovementStrafeRight:
            switch (currentFacing) {
                case SaveFacingNorth: result.deltaCol = 1; break;
                case SaveFacingSouth: result.deltaCol = -1; break;
                case SaveFacingEast: result.deltaRow = 1; break;
                case SaveFacingWest: result.deltaRow = -1; break;
            }
            return result;
    }

    return result;
}

static const MovementBounds kBoundsYendor2 = {0x28, 0x2F7, 0x18, 0x77};
static const MovementBounds kBoundsYendor3 = {0x28, 0x2F7, 0x18, 0x8F};

const MovementBounds *movementBounds(GameKind game) {
    return game == GameYendor3 ? &kBoundsYendor3 : &kBoundsYendor2;
}

bool movementInBounds(GameKind game, uint16_t col, uint16_t row) {
    const MovementBounds *b = movementBounds(game);
    return col >= b->colMin && col <= b->colMax && row >= b->rowMin && row <= b->rowMax;
}

/*
 * ClassifyFloorType (yendor2.asm:1810, yendor3.asm:5474). Both games share
 * the same 3-way shape -- {0,1} is the map's own void/border band
 * (errorCode 2, silent block), then an alternating blocked/normal/blocked
 * sequence (errorCode 1 / 0 / 1) -- but the exact bands differ, and neither
 * is a simple "low blocked, high normal" split:
 *   Chapter 2: void {0,1}; blocked [2,15]; normal [16,57]; blocked (58+,
 *     unbounded -- anything past the real 58-entry wall-type table is
 *     also treated as blocked, not normal).
 *   Chapter 3: void {0,1}; blocked [2,99]; normal [100,199]; blocked
 *     [200,299]; normal (300+, unbounded). The second blocked band,
 *     200-299, is exactly Chapter 3's special-entry range (see
 *     movementIsSpecialWallType) -- structurally the same relationship as
 *     Chapter 2's special range 6-11 sitting inside its 2-15 blocked band.
 */
MovementFloorClass movementClassifyFloorType(GameKind game, uint16_t wallType) {
    if (game == GameYendor3) {
        if (wallType <= 1) return MovementFloorVoid;
        if (wallType <= 99) return MovementFloorBlocked;
        if (wallType <= 199) return MovementFloorNormal;
        if (wallType <= 299) return MovementFloorBlocked;
        return MovementFloorNormal;
    }
    if (wallType <= 1) return MovementFloorVoid;
    if (wallType <= 15) return MovementFloorBlocked;
    if (wallType <= 57) return MovementFloorNormal;
    return MovementFloorBlocked;
}

/*
 * IsCellTypeImpassable (yendor2.asm:1849, yendor3.asm:5505): a second,
 * independent check against the cell's floor/overlay type (worldMapTileB),
 * consulted only once ClassifyFloorType has passed the wall type as
 * MovementFloorNormal. Chapter 3's impassable range is a single contiguous
 * band, 200-399; Chapter 2's is three disjoint pieces -- 21-35, 37 alone
 * (36 and 38 are passable), and 39-42 -- not a single range.
 */
bool movementIsFloorTypeImpassable(GameKind game, uint16_t floorType) {
    if (game == GameYendor3) {
        return floorType >= 200 && floorType <= 399;
    }
    if (floorType >= 21 && floorType <= 35) return true;
    if (floorType == 37) return true;
    if (floorType >= 39 && floorType <= 42) return true;
    return false;
}

/*
 * The special-cell-entry range, read directly from each game's
 * InitGlobals: Chapter 2's _val31/_val32 are 6/11; Chapter 3's
 * ds:5450h/5452h are 0x12B/0xC8 (300-1=0x12B, 200=0xC8), i.e. 200-299.
 */
bool movementIsSpecialWallType(GameKind game, uint16_t wallType) {
    if (game == GameYendor3) {
        return wallType >= 200 && wallType <= 299;
    }
    return wallType >= 6 && wallType <= 11;
}

MovementCellOutcome movementClassifyCell(GameKind game, uint16_t wallType, uint16_t floorType, bool isDoor,
                                          bool forceMove) {
    if (isDoor) {
        return MovementCellDoor;
    }
    if (movementIsSpecialWallType(game, wallType)) {
        return MovementCellSpecial;
    }
    if (forceMove) {
        return MovementCellClear;
    }

    MovementFloorClass floorClass = movementClassifyFloorType(game, wallType);
    if (floorClass == MovementFloorVoid) {
        return MovementCellVoid;
    }
    if (floorClass == MovementFloorBlocked) {
        return MovementCellBlocked;
    }
    if (movementIsFloorTypeImpassable(game, floorType)) {
        return MovementCellBlocked;
    }
    return MovementCellClear;
}

#include "spelljump.h"

#include "movement.h"
#include "savegame.h"
#include "spellrecord.h"

typedef enum { DirForward, DirBackward, DirLeft, DirRight } Dir;

/* Cell delta for a relative direction at a facing; false for a facing that isn't exactly one of the four bits. */
static bool relativeDelta(Dir dir, uint16_t facing, int *dc, int *dr) {
    /* facing as a clockwise index: north 0, east 1, south 2, west 3 */
    int index;
    switch (facing) {
    case SaveFacingNorth: index = 0; break;
    case SaveFacingEast: index = 1; break;
    case SaveFacingSouth: index = 2; break;
    case SaveFacingWest: index = 3; break;
    default: return false;
    }
    int rotation = dir == DirForward ? 0 : dir == DirRight ? 1 : dir == DirBackward ? 2 : 3;
    static const int cols[4] = {0, 1, 0, -1};
    static const int rows[4] = {-1, 0, 1, 0};
    int absolute = (index + rotation) & 3;
    *dc = cols[absolute];
    *dr = rows[absolute];
    return true;
}

static bool cellOk(GameKind game, SpellJumpCellFn cell, void *context, int col, int row, bool forceClassify) {
    uint16_t wall, floor;
    if (!cell(context, col, row, &wall, &floor)) {
        return false;
    }
    if ((forceClassify || (wall != 0 && wall != 1)) && movementClassifyFloorType(game, wall) != MovementFloorNormal) {
        return false;
    }
    return !movementIsFloorTypeImpassable(game, floor);
}

SpellJumpResult spellResolveJump(GameKind game, const uint8_t *spellRecord, uint16_t facing, int partyCol, int partyRow,
                                  SpellJumpCellFn cell, void *context) {
    SpellJumpResult result = {false, partyCol, partyRow};
    static const struct {
        unsigned field;
        Dir dir;
    } walks[] = {
        {SpellFieldJumpForward, DirForward},
        {SpellFieldJumpBackward, DirBackward},
        {SpellFieldJumpLeft, DirLeft},
        {SpellFieldJumpRight, DirRight},
    };
    for (unsigned i = 0; i < sizeof(walks) / sizeof(walks[0]); i++) {
        uint16_t steps = spellGetU16(spellRecord, walks[i].field);
        if (steps == 0) {
            continue;
        }
        int dc, dr;
        if (!relativeDelta(walks[i].dir, facing, &dc, &dr)) {
            /* the original's chain falls through to its last direction for any unrecognised facing; treat as west */
            if (!relativeDelta(walks[i].dir, SaveFacingWest, &dc, &dr)) {
                return result;
            }
        }
        int col = partyCol, row = partyRow;
        for (unsigned step = 1; step <= steps; step++) {
            col += dc;
            row += dr;
            if (!cellOk(game, cell, context, col, row, step == steps)) {
                return result;
            }
        }
        result.ok = true;
        result.col = col;
        result.row = row;
        return result;
    }

    uint16_t through = spellGetU16(spellRecord, SpellFieldJumpThrough);
    int dc, dr;
    if (through == 0 || !relativeDelta(DirForward, facing, &dc, &dr)) {
        return result;
    }
    int col = partyCol + dc * through, row = partyRow + dr * through;
    if (!cellOk(game, cell, context, col, row, true)) {
        return result;
    }
    result.ok = true;
    result.col = col;
    result.row = row;
    return result;
}

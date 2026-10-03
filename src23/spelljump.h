#ifndef YENDOR23_SPELLJUMP_H
#define YENDOR23_SPELLJUMP_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * The party-jump half of ApplyEncodedItemEffect's SpellBranchTeleportEngage
 * (word_33302 bit 0x4, loc_2C344..loc_2C400/loc_2C558, yendor2.asm:51398,
 * instruction-identical in Chapter 3; JUMP OVER and JUMP THROUGH). Decides
 * where the party lands; the original then moves it, reveals the cells
 * around it, and -- if a monster stands on the destination -- pulls that
 * monster out of the level pool into a combat slot (not modeled).
 *
 * The record's jump words (spellrecord.h, SpellFieldJump*) are tried in the
 * order forward, backward, left, right, through; the first nonzero one
 * is used and the rest ignored. All distances are in cells.
 *
 * Forward/backward/left/right WALK the path one cell at a time, checking
 * each cell: the wall type must classify as MovementFloorNormal
 * (movementClassifyFloorType) -- except that an *intermediate* cell whose
 * wall type is 0 or 1 skips that check (the map's border-band types; the
 * last cell is always classified) -- and the floor type must not be
 * impassable. Any failing cell, or one the map doesn't have, fails the whole
 * jump with no movement. Through does a single hop of that many cells in the
 * facing direction and checks only the destination cell, the same way.
 * No nonzero word, or an unrecognised facing for Through, fails too.
 *
 * Directions for facing north (0x8000): forward is row-1, backward row+1,
 * left col-1, right col+1; the others rotate with the facing.
 */
typedef bool (*SpellJumpCellFn)(void *context, int col, int row, uint16_t *wallType, uint16_t *floorType);

typedef struct {
    bool ok;
    int col, row; /* the landing cell; meaningful only when ok */
} SpellJumpResult;

SpellJumpResult spellResolveJump(GameKind game, const uint8_t *spellRecord, uint16_t facing, int partyCol, int partyRow,
                                  SpellJumpCellFn cell, void *context);

#endif

#ifndef YENDOR23_EXPLORE_H
#define YENDOR23_EXPLORE_H

#include <stdbool.h>
#include <stdint.h>

#include "savegame.h"

/*
 * The fog-of-war bitmap (CURGAME section 1, SaveSectionExploredMap: one record per world row, one bit per
 * column, most significant bit first -- bit (7 - x % 8) of byte x / 8) and the movement-time reveal.
 *
 * MarkCellExplored (yendor2.asm:31591, instruction-identical in Chapter 3): sets a cell's explored flag and
 * persists the bit (PersistExploredCell, :31472); a no-op when already set. RevealCellsAroundPlayer (:31512)
 * runs after every step: it marks two rows of three cells beside the party's facing -- facing north: the row
 * at y - 1, then the row at y; south: y + 1, then y; east: the column x + 1, then x; west (the default):
 * x - 1, then x -- each row/column's cells in the order -1, +1, 0 relative to the party. So the party always sees
 * its own row/column and the one ahead, three cells wide.
 */
bool exploreIsExplored(SaveGame *save, int x, int y);

/* Returns true when the cell was newly explored (false when already explored or outside the bitmap). */
bool exploreMarkCell(SaveGame *save, int x, int y);

typedef struct {
    unsigned count;
    struct {
        int x, y;
    } cells[6];
} ExploreReveal;

/* Marks the six cells in the original's order; *newly lists those that were not yet explored. */
void exploreRevealAroundPlayer(SaveGame *save, int x, int y, uint16_t facing, ExploreReveal *newly);

#endif

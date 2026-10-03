#ifndef YENDOR23_VIEWPORT_H
#define YENDOR23_VIEWPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "dungeongrid.h"

/*
 * The first-person view's cell buffer and its occlusion pass: BuildDungeonViewportCells (yendor2.asm:30343,
 * yendor3.asm:29494), CopyDungeonRowCells, ComputeDungeonCellVisibility (:30448) and IsDungeonRowFullyBlocked
 * (:30560), called at the start of every redraw. Together they decide which of the 51 cells in front of the party are
 * drawn.
 *
 * The buffer holds 51 copies of grid cells (8 bytes each, flags at +6) in seven rows, FAR to NEAR:
 *   row 0 cells 0-16 (17 wide)   row 1 17-33 (17)   row 2 34-38 (5)   row 3 39-41 (3)   row 4 42-44   row 5 45-47
 *   row 6 48-50 (the party's own row; cell 49 is the party's cell)
 * Rows 0-1 are the two far rows spanning eight cells either side, row 2 five cells, the rest three. In world terms, per
 * facing, row 0's first cell is at party + start, each cell then steps by `column` and each row by `row`:
 *   north  start (-8, -6)  column (+1, 0)   row (0, +1)        south  start (+8, +6)  column (-1, 0)  row (0, -1)
 *   east   start (+6, -8)  column (0, +1)   row (-1, 0)        west   start (-6, +8)  column (0, -1)  row (+1, 0)
 * (rows 2.. begin 6 columns in, rows 3.. 7 columns in).
 *
 * Occlusion sets bit 0 of a cell's flags ("hidden", in the copy only):
 *   1. the first of these rows that is entirely solid wall hides everything farther: row 5 (cells 45-47) -> hides 0-44;
 *      else row 4 (42-44) -> 0-41; else row 3 (39-41) -> 0-38; else row 2 (34-38) -> 0-33; else row 0..1 as one 17-wide
 *      run starting at cell 17 -> 0-16. A "solid" cell is not already hidden and has wall type in
 *      [2, 5] (Chapter 2) / [2, 99] (Chapter 3).
 *   2. eight (cell, list) rules: when a cell and its right-hand neighbour (the next cell in the buffer) are both solid
 *      and visible, every cell in the list is hidden (walls hiding what stands behind a corner).
 *   3. for cells 50 down to 18: a visible solid cell hides its own list (a wall hides the cells directly behind it).
 * The tables are identical in both games (the originals store buffer addresses; here they are cell indices).
 */
enum { ViewportCellCount = 51, ViewportCellHidden = 1, ViewportRowCount = 7 };

void viewportBuild(const DungeonGrid *grid, uint16_t facing, int partyX, int partyY, DungeonGridCell out[ViewportCellCount]);

/* World position of buffer cell `index` (for tests and for callers that map cells back to the map). */
void viewportCellWorld(uint16_t facing, int partyX, int partyY, unsigned index, int *x, int *y);

void viewportComputeVisibility(GameKind game, DungeonGridCell cells[ViewportCellCount]);

#endif

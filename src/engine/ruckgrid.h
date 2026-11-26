/*
 * ruckgrid.h - cursor movement of the whole-rucksack grid (1000:125CF, the key handling 2746 .. 2825), verified against the original (tests/vectors_grid.h).
 * The grid shows four things per row and at most four rows; the cursor is (column gx, absolute row grow, visible row gvis) and gscroll is the first visible row.
 */
#ifndef CADAVER_RUCKGRID_H
#define CADAVER_RUCKGRID_H

/* key: 1 up, 2 down, 4 left, 8 right; n = number of carried things (1..32). Transcribed with the original's quirks (see docs/formats.md). */
void ruckgrid_nav(int key, int n, int *gx, int *grow, int *gvis, int *gscroll);

#endif

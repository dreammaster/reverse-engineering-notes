/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_viewport test_viewport.c ../viewport.c ../dungeongrid.c ../movement.c ../worldmap.c ../worldmap_stdio.c ../savegame.c && ./test_viewport
 */
#include <stdio.h>
#include <string.h>

#include "viewport.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void testGeometry(void) {
    int x, y;
    viewportCellWorld(SaveFacingNorth, 100, 50, 0, &x, &y);
    check("north: cell 0 is 8 west and 6 north", x == 92 && y == 44);
    viewportCellWorld(SaveFacingNorth, 100, 50, 16, &x, &y);
    check("...cell 16 ends the far row 8 east", x == 108 && y == 44);
    viewportCellWorld(SaveFacingNorth, 100, 50, 17, &x, &y);
    check("...cell 17 starts the second far row", x == 92 && y == 45);
    viewportCellWorld(SaveFacingNorth, 100, 50, 34, &x, &y);
    check("...cell 34 starts the 5-wide row 2 cells west, 2 rows ahead", x == 98 && y == 46);
    viewportCellWorld(SaveFacingNorth, 100, 50, 49, &x, &y);
    check("...cell 49 is the party's own cell", x == 100 && y == 50);
    viewportCellWorld(SaveFacingNorth, 100, 50, 45, &x, &y);
    check("...cell 45 is one ahead and one to the left", x == 99 && y == 49);
    for (unsigned f = 0; f < 4; f++) {
        uint16_t facing = f == 0 ? SaveFacingNorth : f == 1 ? SaveFacingSouth : f == 2 ? SaveFacingEast : SaveFacingWest;
        viewportCellWorld(facing, 100, 50, 49, &x, &y);
        if (x != 100 || y != 50) {
            check("cell 49 is the party's in every facing", false);
        }
    }
    viewportCellWorld(SaveFacingSouth, 100, 50, 0, &x, &y);
    check("south: cell 0 is 8 east and 6 south", x == 108 && y == 56);
    viewportCellWorld(SaveFacingEast, 100, 50, 0, &x, &y);
    check("east: cell 0 is 6 east and 8 north", x == 106 && y == 42);
    viewportCellWorld(SaveFacingEast, 100, 50, 16, &x, &y);
    check("...its far row runs south", x == 106 && y == 58);
    viewportCellWorld(SaveFacingEast, 100, 50, 45, &x, &y);
    check("...cell 45 is one cell east and one north", x == 101 && y == 49);
    viewportCellWorld(SaveFacingWest, 100, 50, 0, &x, &y);
    check("west: cell 0 is 6 west and 8 south", x == 94 && y == 58);
    viewportCellWorld(SaveFacingWest, 100, 50, 45, &x, &y);
    check("...cell 45 is one cell west and one south", x == 99 && y == 51);
}

static void testBuild(void) {
    static DungeonGrid grid;
    memset(&grid, 0, sizeof(grid));
    grid.game = GameYendor2;
    grid.originCol = 60;
    grid.originRow = 20;
    /* mark every cell with its world column*1000+row in floorType so the copies are traceable */
    for (int r = 0; r < DungeonGridSize; r++) {
        for (int c = 0; c < DungeonGridSize; c++) {
            grid.cells[r][c].wallType = (uint16_t)(grid.originCol + c);
            grid.cells[r][c].floorType = (uint16_t)(grid.originRow + r);
        }
    }
    DungeonGridCell cells[ViewportCellCount];
    viewportBuild(&grid, SaveFacingNorth, 100, 50, cells);
    check("the buffer copies the grid cells at each position", cells[0].wallType == 92 && cells[0].floorType == 44 && cells[49].wallType == 100 &&
                                                                  cells[49].floorType == 50 && cells[50].wallType == 101);
    viewportBuild(&grid, SaveFacingWest, 100, 50, cells);
    check("(west)", cells[0].wallType == 94 && cells[0].floorType == 58 && cells[49].wallType == 100);
    viewportBuild(&grid, SaveFacingNorth, 60, 20, cells);
    check("cells outside the window read as empty", cells[0].wallType == 0 && cells[0].floorType == 0 && cells[49].wallType == 60);
}

static void fill(DungeonGridCell cells[ViewportCellCount], uint16_t wall) {
    for (unsigned i = 0; i < ViewportCellCount; i++) {
        memset(&cells[i], 0, sizeof(cells[i]));
        cells[i].wallType = wall;
    }
}

static unsigned hidden(const DungeonGridCell cells[ViewportCellCount]) {
    unsigned n = 0;
    for (unsigned i = 0; i < ViewportCellCount; i++) {
        n += cells[i].flags & ViewportCellHidden ? 1 : 0;
    }
    return n;
}

static void testVisibility(void) {
    DungeonGridCell cells[ViewportCellCount];
    fill(cells, 0);
    viewportComputeVisibility(GameYendor2, cells);
    check("an open corridor hides nothing", hidden(cells) == 0);

    fill(cells, 0);
    for (unsigned i = 45; i <= 47; i++) {
        cells[i].wallType = 3;
    }
    viewportComputeVisibility(GameYendor2, cells);
    bool allFar = true;
    for (unsigned i = 0; i < 45; i++) {
        allFar = allFar && (cells[i].flags & ViewportCellHidden);
    }
    check("a solid row right ahead hides everything beyond it", allFar && !(cells[45].flags & ViewportCellHidden));

    fill(cells, 0);
    for (unsigned i = 45; i <= 47; i++) {
        cells[i].wallType = 6; /* outside Chapter 2's solid range 2-5 */
    }
    viewportComputeVisibility(GameYendor2, cells);
    check("a wall type outside [2, 5] is not solid in Chapter 2...", hidden(cells) == 0);
    viewportComputeVisibility(GameYendor3, cells);
    check("...but is in Chapter 3 (range 2-99)", hidden(cells) >= 45);

    fill(cells, 0);
    for (unsigned i = 42; i <= 44; i++) {
        cells[i].wallType = 2;
    }
    viewportComputeVisibility(GameYendor2, cells);
    check("the next row out: a solid row two ahead hides cells 0-41", hidden(cells) >= 42 && (cells[41].flags & 1) && !(cells[42].flags & 1));

    fill(cells, 0);
    cells[45].wallType = 4;
    cells[46].wallType = 4;
    viewportComputeVisibility(GameYendor2, cells);
    check("a pair rule: cells 45 and 46 solid hide the cells in the corner list", (cells[0].flags & 1) && (cells[19].flags & 1) && (cells[42].flags & 1));
    fill(cells, 0);
    cells[45].wallType = 4;
    viewportComputeVisibility(GameYendor2, cells);
    check("...but not with the neighbour open", !(cells[19].flags & 1));

    fill(cells, 0);
    cells[27].wallType = 3; /* a lone wall: hides the cells directly behind it (table row 27 - 17) */
    viewportComputeVisibility(GameYendor2, cells);
    check("a lone solid cell hides the cells it shadows", hidden(cells) > 0 && !(cells[27].flags & 1));
}

int main(void) {
    testGeometry();
    testBuild();
    testVisibility();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

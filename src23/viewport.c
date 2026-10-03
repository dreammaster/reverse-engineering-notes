#include "viewport.h"

#include <string.h>

typedef struct {
    uint8_t cell;
    uint8_t hide[22]; /* 255 ends the list */
} HideRule;

static const HideRule kPairRules[] = {
    {45, {0, 19, 20, 21, 22, 23, 34, 35, 39, 42, 255}},
    {46, {16, 27, 28, 29, 30, 31, 37, 38, 41, 44, 255}},
    {42, {0, 1, 2, 17, 18, 21, 22, 23, 35, 39, 255}},
    {43, {14, 15, 16, 32, 33, 27, 28, 29, 37, 41, 255}},
    {39, {0, 1, 2, 3, 4, 5, 6, 7, 22, 23, 24, 35, 255}},
    {40, {9, 10, 11, 12, 13, 14, 15, 16, 26, 27, 28, 37, 255}},
    {35, {2, 3, 4, 5, 6, 7, 23, 24, 255}},
    {36, {9, 10, 11, 12, 13, 14, 26, 27, 255}}
};

/* kRowLists[c - 17]: what a visible solid cell c (18..50) hides; {255} = nothing. */
static const uint8_t kRowLists[34][22] = {
    {255},
    {255},
    {255},
    {255},
    {255},
    {0, 255},
    {0, 1, 2, 255},
    {255},
    {8, 255},
    {255},
    {14, 15, 16, 255},
    {16, 255},
    {255},
    {255},
    {255},
    {255},
    {255},
    {17, 18, 19, 255},
    {0, 1, 17, 18, 19, 20, 21, 22, 255},
    {8, 25, 255},
    {15, 16, 28, 29, 30, 31, 32, 33, 255},
    {31, 32, 33, 255},
    {17, 18, 19, 20, 21, 34, 255},
    {8, 25, 36, 255},
    {29, 30, 31, 32, 33, 38, 255},
    {17, 18, 19, 20, 34, 255},
    {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 24, 25, 26, 36, 40, 255},
    {30, 31, 32, 33, 38, 255},
    {17, 18, 255},
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 36, 40, 43, 255},
    {33, 32, 255},
    {17, 255},
    {255},
    {33, 255}
};

static bool isSolid(const DungeonGridCell *cell, uint16_t low, uint16_t high) {
    return !(cell->flags & ViewportCellHidden) && cell->wallType >= low && cell->wallType <= high;
}

static void hideList(DungeonGridCell cells[ViewportCellCount], const uint8_t *list) {
    for (; *list != 255; list++) {
        cells[*list].flags |= ViewportCellHidden;
    }
}

/* IsDungeonRowFullyBlocked: cells [first, first + count) all solid -> hide cells 0..hideCount-1. */
static bool rowFullyBlocked(DungeonGridCell cells[ViewportCellCount], unsigned first, unsigned count, unsigned hideCount, uint16_t low,
                            uint16_t high) {
    for (unsigned i = 0; i < count; i++) {
        if (!isSolid(&cells[first + i], low, high)) {
            return false;
        }
    }
    for (unsigned i = 0; i < hideCount; i++) {
        cells[i].flags |= ViewportCellHidden;
    }
    return true;
}

typedef struct {
    int startX, startY, columnX, columnY, rowX, rowY;
} FacingGeometry;

static FacingGeometry geometryFor(uint16_t facing) {
    if (facing == SaveFacingNorth) {
        return (FacingGeometry){-8, -6, 1, 0, 0, 1};
    }
    if (facing == SaveFacingSouth) {
        return (FacingGeometry){8, 6, -1, 0, 0, -1};
    }
    if (facing == SaveFacingEast) {
        return (FacingGeometry){6, -8, 0, 1, -1, 0};
    }
    return (FacingGeometry){-6, 8, 0, -1, 1, 0};
}

void viewportCellWorld(uint16_t facing, int partyX, int partyY, unsigned index, int *x, int *y) {
    static const uint8_t kRowFirst[ViewportRowCount] = {0, 17, 34, 39, 42, 45, 48};
    static const uint8_t kRowColumnShift[ViewportRowCount] = {0, 0, 6, 7, 7, 7, 7};
    FacingGeometry g = geometryFor(facing);
    unsigned row = 0;
    while (row + 1 < ViewportRowCount && index >= kRowFirst[row + 1]) {
        row++;
    }
    int column = (int)(index - kRowFirst[row]) + kRowColumnShift[row];
    *x = partyX + g.startX + g.columnX * column + g.rowX * (int)row;
    *y = partyY + g.startY + g.columnY * column + g.rowY * (int)row;
}

void viewportBuild(const DungeonGrid *grid, uint16_t facing, int partyX, int partyY, DungeonGridCell out[ViewportCellCount]) {
    for (unsigned i = 0; i < ViewportCellCount; i++) {
        int x, y;
        viewportCellWorld(facing, partyX, partyY, i, &x, &y);
        const DungeonGridCell *cell = dungeonGridCellAtWorldPos(grid, x, y);
        if (cell) {
            out[i] = *cell;
        } else {
            memset(&out[i], 0, sizeof(out[i]));
        }
    }
}

void viewportComputeVisibility(GameKind game, DungeonGridCell cells[ViewportCellCount]) {
    uint16_t low = 2, high = game == GameYendor3 ? 99 : 5;
    if (!rowFullyBlocked(cells, 45, 3, 45, low, high) && !rowFullyBlocked(cells, 42, 3, 42, low, high) &&
        !rowFullyBlocked(cells, 39, 3, 39, low, high) && !rowFullyBlocked(cells, 34, 5, 34, low, high)) {
        rowFullyBlocked(cells, 17, 17, 17, low, high);
    }
    for (unsigned i = 0; i < sizeof(kPairRules) / sizeof(kPairRules[0]); i++) {
        unsigned c = kPairRules[i].cell;
        if (isSolid(&cells[c], low, high) && isSolid(&cells[c + 1], low, high)) {
            hideList(cells, kPairRules[i].hide);
        }
    }
    for (unsigned c = 50; c >= 18; c--) {
        if (isSolid(&cells[c], low, high)) {
            hideList(cells, kRowLists[c - 17]);
        }
    }
}

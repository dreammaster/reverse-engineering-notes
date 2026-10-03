#include "worldmap.h"

#include <string.h>

static const WorldMapLayout g_layoutYendor2 = {0, WorldMapRowsYendor2};
static const WorldMapLayout g_layoutYendor3 = {0, WorldMapRowsYendor3};

const WorldMapLayout *worldMapLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool worldMapParse(WorldMap *map, GameKind game, const uint8_t *region, size_t size) {
    const WorldMapLayout *layout = worldMapLayout(game);
    if (!layout || size < (size_t)layout->rowCount * WorldMapRowSize) {
        return false;
    }
    memset(map, 0, sizeof(*map));
    map->game = game;
    map->rowCount = layout->rowCount;
    memcpy(map->rows, region, (size_t)layout->rowCount * WorldMapRowSize);
    return true;
}

bool worldMapParseWorldDat(WorldMap *map, GameKind game, const uint8_t *worldDat, size_t size) {
    const WorldMapLayout *layout = worldMapLayout(game);
    if (!layout) {
        return false;
    }
    size_t needed = (size_t)layout->offset + (size_t)layout->rowCount * WorldMapRowSize;
    if (size < needed) {
        return false;
    }
    return worldMapParse(map, game, worldDat + layout->offset, size - layout->offset);
}

const uint8_t *worldMapCell(const WorldMap *map, unsigned row, unsigned col) {
    if (row >= map->rowCount || col >= WorldMapColumns) {
        return NULL;
    }
    return map->rows + (size_t)row * WorldMapRowSize + (size_t)col * WorldMapColumnSize;
}

static uint16_t readU16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint16_t worldMapTileA(const WorldMap *map, unsigned row, unsigned col) {
    const uint8_t *cell = worldMapCell(map, row, col);
    return cell ? readU16(cell) : 0;
}

uint16_t worldMapTileB(const WorldMap *map, unsigned row, unsigned col) {
    const uint8_t *cell = worldMapCell(map, row, col);
    return cell ? readU16(cell + 2) : 0;
}

/* generated from ida_scripts/dump_tile_legends.py output (both games) */

static const uint16_t kWallLegendYendor2[58][6] = {
    {0, 0, 0, 0, 0, 22},
    {1, 0, 0, 0, 0, 23},
    {0, 0, 52, 22, 0, 24},
    {0, 0, 19, 22, 14, 27},
    {0, 0, 25, 22, 28, 30},
    {0, 0, 65, 22, 42, 32},
    {0, 0, 61, 22, 0, 34},
    {0, 0, 46, 22, 0, 24},
    {0, 0, 59, 22, 14, 34},
    {0, 0, 44, 22, 14, 27},
    {0, 0, 9, 22, 28, 80},
    {0, 0, 66, 22, 42, 34},
    {0, 0, 0, 22, 0, 24},
    {0, 0, 0, 22, 14, 27},
    {0, 0, 0, 22, 28, 30},
    {0, 0, 0, 22, 42, 32},
    {4, 0, 0, 0, 0, 37},
    {5, 0, 0, 0, 0, 38},
    {6, 0, 0, 0, 0, 39},
    {7, 0, 0, 0, 0, 40},
    {2, 0, 0, 0, 0, 41},
    {3, 0, 0, 0, 0, 41},
    {12, 0, 0, 0, 0, 42},
    {13, 0, 0, 0, 0, 42},
    {10, 0, 0, 0, 0, 44},
    {11, 0, 0, 0, 0, 45},
    {4, 4, 0, 0, 0, 37},
    {5, 5, 0, 0, 0, 38},
    {12, 4, 0, 0, 0, 42},
    {13, 5, 0, 0, 0, 42},
    {2, 4, 0, 0, 0, 41},
    {3, 5, 0, 0, 0, 41},
    {10, 4, 0, 0, 0, 44},
    {11, 5, 0, 0, 0, 45},
    {12, 2, 0, 0, 0, 42},
    {13, 3, 0, 0, 0, 42},
    {2, 2, 0, 0, 0, 41},
    {3, 3, 0, 0, 0, 41},
    {8, 0, 0, 0, 0, 71},
    {8, 0, 0, 0, 0, 71},
    {8, 4, 0, 0, 0, 71},
    {8, 5, 0, 0, 0, 71},
    {8, 2, 0, 0, 0, 71},
    {8, 3, 0, 0, 0, 71},
    {4, 8, 0, 0, 0, 37},
    {5, 9, 0, 0, 0, 38},
    {12, 8, 0, 0, 0, 42},
    {13, 9, 0, 0, 0, 42},
    {10, 8, 0, 0, 0, 44},
    {11, 9, 0, 0, 0, 45},
    {4, 6, 0, 0, 0, 37},
    {5, 7, 0, 0, 0, 38},
    {14, 10, 0, 0, 0, 36},
    {15, 11, 0, 0, 0, 36},
    {16, 4, 0, 0, 0, 15},
    {17, 5, 0, 0, 0, 16},
    {16, 0, 0, 0, 0, 15},
    {17, 0, 0, 0, 0, 16}
};

static const uint16_t kFloorLegendYendor2[68][5] = {
    {0, 0, 0, 0, 0},
    {43, 43, 43, 43, 49},
    {28, 26, 32, 32, 64},
    {29, 27, 31, 30, 64},
    {26, 28, 32, 32, 64},
    {27, 29, 30, 31, 64},
    {32, 32, 28, 26, 65},
    {30, 31, 29, 27, 65},
    {32, 32, 26, 28, 65},
    {31, 30, 27, 29, 65},
    {11, 11, 11, 11, 66},
    {12, 12, 12, 12, 66},
    {55, 55, 55, 55, 57},
    {14, 13, 15, 16, 58},
    {13, 14, 16, 15, 59},
    {15, 16, 13, 14, 60},
    {16, 15, 14, 13, 61},
    {71, 71, 71, 71, 81},
    {72, 72, 72, 72, 82},
    {41, 41, 41, 41, 83},
    {42, 42, 42, 42, 84},
    {69, 69, 69, 69, 33},
    {70, 70, 70, 70, 33},
    {51, 51, 51, 51, 25},
    {50, 50, 50, 50, 26},
    {33, 33, 33, 33, 34},
    {53, 53, 53, 53, 43},
    {18, 18, 18, 18, 28},
    {17, 17, 17, 17, 29},
    {20, 20, 20, 20, 43},
    {24, 24, 24, 24, 31},
    {34, 34, 34, 34, 35},
    {64, 64, 64, 64, 43},
    {68, 68, 68, 68, 85},
    {23, 23, 23, 23, 52},
    {37, 37, 37, 37, 55},
    {57, 57, 57, 57, 46},
    {58, 58, 58, 58, 47},
    {54, 54, 54, 54, 48},
    {21, 21, 21, 21, 50},
    {22, 22, 22, 22, 51},
    {35, 35, 35, 35, 53},
    {36, 36, 36, 36, 54},
    {63, 63, 63, 63, 56},
    {38, 38, 38, 38, 62},
    {48, 48, 48, 48, 63},
    {49, 49, 49, 49, 63},
    {56, 0, 0, 0, 67},
    {0, 56, 0, 0, 68},
    {0, 0, 56, 0, 69},
    {0, 0, 0, 56, 70},
    {83, 83, 83, 83, 86},
    {84, 84, 84, 84, 86},
    {85, 85, 85, 85, 86},
    {86, 86, 86, 86, 86},
    {87, 87, 87, 87, 87},
    {88, 88, 88, 88, 86},
    {89, 89, 89, 89, 86},
    {90, 90, 90, 90, 87},
    {91, 91, 91, 91, 86},
    {92, 92, 92, 92, 86},
    {93, 93, 93, 93, 86},
    {94, 94, 94, 94, 88},
    {42, 42, 42, 42, 84},
    {72, 72, 72, 72, 82},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
};

static const uint16_t kWallLegendYendor3Page0[10][6] = {
    {0, 0, 0, 0, 0, 29},
    {1, 0, 0, 0, 0, 30},
    {0, 0, 120, 22, 0, 31},
    {0, 0, 71, 22, 14, 66},
    {0, 0, 90, 22, 28, 83},
    {0, 0, 96, 22, 0, 31},
    {0, 0, 26, 22, 42, 92},
    {0, 0, 127, 23, 0, 122},
    {0, 0, 143, 23, 14, 135},
    {0, 0, 0, 22, 0, 0}
};

static const uint16_t kWallLegendYendor3Page1[4][6] = {
    {0, 0, 0, 22, 0, 0},
    {0, 0, 0, 22, 14, 0},
    {0, 0, 0, 22, 28, 0},
    {4, 0, 14, 22, 0, 32}
};

static const uint16_t kWallLegendYendor3Page2[9][6] = {
    {4, 0, 14, 22, 0, 32},
    {0, 0, 23, 22, 0, 52},
    {13, 0, 72, 22, 14, 32},
    {13, 0, 91, 22, 28, 84},
    {0, 0, 109, 22, 14, 102},
    {0, 0, 111, 22, 28, 103},
    {13, 0, 128, 23, 0, 32},
    {0, 0, 130, 23, 0, 123},
    {2, 0, 0, 0, 0, 33}
};

static const uint16_t kWallLegendYendor3Page3[43][6] = {
    {2, 0, 0, 0, 0, 33},
    {3, 0, 0, 0, 0, 34},
    {8, 0, 0, 0, 0, 38},
    {9, 0, 0, 0, 0, 39},
    {4, 4, 0, 0, 0, 48},
    {5, 5, 0, 0, 0, 48},
    {12, 0, 0, 0, 0, 54},
    {13, 0, 0, 0, 0, 55},
    {10, 0, 0, 0, 0, 64},
    {11, 0, 0, 0, 0, 65},
    {14, 0, 0, 0, 0, 80},
    {15, 0, 0, 0, 0, 80},
    {16, 4, 0, 0, 0, 81},
    {17, 5, 0, 0, 0, 81},
    {4, 8, 0, 0, 0, 48},
    {5, 9, 0, 0, 0, 48},
    {18, 6, 0, 0, 0, 87},
    {19, 7, 0, 0, 0, 88},
    {14, 6, 0, 0, 0, 80},
    {15, 7, 0, 0, 0, 80},
    {18, 10, 0, 0, 0, 87},
    {19, 11, 0, 0, 0, 88},
    {6, 6, 0, 0, 0, 38},
    {7, 7, 0, 0, 0, 39},
    {16, 0, 0, 0, 0, 81},
    {17, 0, 0, 0, 0, 81},
    {6, 10, 0, 0, 0, 38},
    {7, 11, 0, 0, 0, 39},
    {20, 0, 0, 0, 0, 115},
    {21, 0, 0, 0, 0, 116},
    {22, 0, 0, 0, 0, 124},
    {23, 0, 0, 0, 0, 125},
    {24, 0, 0, 0, 0, 126},
    {25, 0, 0, 0, 0, 127},
    {22, 6, 0, 0, 0, 124},
    {23, 7, 0, 0, 0, 125},
    {24, 6, 0, 0, 0, 126},
    {25, 7, 0, 0, 0, 127},
    {20, 6, 0, 0, 0, 115},
    {21, 7, 0, 0, 0, 116},
    {26, 12, 0, 0, 0, 133},
    {27, 13, 0, 0, 0, 134},
    {0, 0, 0, 0, 0, 0}
};

static const struct { const uint16_t (*rows)[6]; unsigned count; } kWallPagesYendor3[] = {
    {kWallLegendYendor3Page0, 10},
    {kWallLegendYendor3Page1, 4},
    {kWallLegendYendor3Page2, 9},
    {kWallLegendYendor3Page3, 43}
};

static const uint16_t kFloorLegendYendor3Page0[12][5] = {
    {0, 0, 0, 0, 0},
    {1, 1, 2, 2, 35},
    {5, 5, 6, 6, 40},
    {3, 3, 4, 4, 41},
    {7, 7, 8, 8, 59},
    {9, 9, 9, 9, 0},
    {250, 250, 251, 251, 117},
    {254, 254, 255, 255, 40},
    {252, 252, 252, 252, 118},
    {256, 256, 257, 257, 141},
    {258, 258, 259, 259, 142},
    {16, 17, 18, 19, 36}
};

static const uint16_t kFloorLegendYendor3Page1[72][5] = {
    {16, 17, 18, 19, 36},
    {17, 16, 19, 18, 44},
    {19, 18, 16, 17, 45},
    {18, 19, 17, 16, 46},
    {52, 52, 53, 53, 47},
    {27, 28, 29, 29, 49},
    {30, 28, 32, 31, 49},
    {28, 27, 29, 29, 49},
    {28, 30, 31, 32, 49},
    {29, 29, 27, 28, 50},
    {31, 32, 30, 28, 50},
    {29, 29, 28, 27, 50},
    {32, 31, 28, 30, 50},
    {46, 48, 49, 47, 51},
    {56, 54, 55, 55, 56},
    {54, 56, 55, 55, 56},
    {55, 55, 56, 54, 56},
    {55, 55, 54, 56, 56},
    {57, 57, 58, 58, 57},
    {58, 58, 57, 57, 58},
    {59, 61, 62, 62, 56},
    {61, 59, 62, 62, 56},
    {62, 62, 59, 61, 56},
    {62, 62, 61, 59, 56},
    {60, 54, 55, 55, 56},
    {54, 60, 55, 55, 56},
    {55, 55, 60, 54, 56},
    {55, 55, 54, 60, 56},
    {63, 64, 65, 65, 60},
    {66, 67, 69, 68, 60},
    {64, 63, 65, 65, 60},
    {67, 66, 68, 69, 60},
    {65, 65, 63, 64, 61},
    {68, 69, 66, 67, 61},
    {65, 65, 64, 63, 61},
    {69, 68, 67, 66, 61},
    {86, 86, 86, 86, 70},
    {87, 87, 87, 87, 70},
    {89, 61, 62, 62, 56},
    {61, 89, 62, 62, 56},
    {62, 62, 89, 61, 56},
    {62, 62, 61, 89, 56},
    {105, 105, 105, 105, 91},
    {106, 106, 106, 106, 93},
    {113, 113, 113, 113, 104},
    {0, 0, 0, 0, 113},
    {0, 0, 0, 0, 113},
    {0, 0, 0, 0, 113},
    {0, 0, 0, 0, 113},
    {122, 122, 122, 122, 114},
    {125, 125, 126, 126, 121},
    {136, 54, 55, 55, 56},
    {54, 136, 55, 55, 56},
    {55, 55, 136, 54, 56},
    {55, 55, 54, 136, 56},
    {113, 113, 113, 113, 104},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {138, 138, 139, 139, 129},
    {144, 144, 145, 145, 136},
    {113, 113, 113, 113, 104},
    {113, 113, 113, 113, 104},
    {113, 113, 113, 113, 104},
    {113, 113, 113, 113, 104},
    {113, 113, 113, 113, 104},
    {149, 150, 151, 152, 137},
    {150, 149, 151, 152, 138},
    {152, 151, 149, 150, 139},
    {151, 152, 150, 149, 140},
    {20, 20, 20, 20, 37}
};

static const uint16_t kFloorLegendYendor3Page2[55][5] = {
    {20, 20, 20, 20, 37},
    {50, 50, 50, 50, 42},
    {51, 51, 51, 51, 43},
    {22, 0, 0, 0, 31},
    {0, 22, 0, 0, 31},
    {0, 0, 22, 0, 31},
    {0, 0, 0, 22, 31},
    {21, 21, 21, 21, 32},
    {33, 33, 33, 33, 53},
    {25, 25, 25, 25, 62},
    {70, 70, 70, 70, 63},
    {74, 74, 74, 74, 53},
    {75, 75, 75, 75, 67},
    {76, 76, 76, 76, 69},
    {77, 77, 77, 77, 68},
    {78, 78, 78, 78, 71},
    {79, 79, 79, 79, 72},
    {80, 80, 80, 80, 73},
    {81, 81, 81, 81, 74},
    {82, 82, 82, 82, 75},
    {83, 83, 83, 83, 76},
    {84, 84, 84, 84, 77},
    {85, 85, 85, 85, 78},
    {88, 88, 88, 88, 79},
    {95, 95, 95, 95, 82},
    {94, 94, 94, 94, 85},
    {93, 93, 93, 93, 84},
    {97, 97, 97, 97, 86},
    {98, 98, 98, 98, 89},
    {104, 104, 104, 104, 90},
    {107, 107, 107, 107, 90},
    {108, 108, 108, 108, 53},
    {114, 114, 114, 114, 105},
    {115, 115, 115, 115, 86},
    {116, 116, 116, 116, 110},
    {117, 117, 117, 117, 106},
    {117, 117, 117, 117, 107},
    {117, 117, 117, 117, 108},
    {117, 117, 117, 117, 109},
    {118, 118, 118, 118, 111},
    {119, 119, 119, 119, 112},
    {121, 121, 121, 121, 69},
    {123, 123, 123, 123, 119},
    {124, 124, 124, 124, 120},
    {132, 132, 132, 132, 111},
    {133, 133, 133, 133, 128},
    {137, 137, 137, 137, 31},
    {140, 140, 140, 140, 130},
    {141, 141, 141, 141, 131},
    {142, 142, 142, 142, 132},
    {146, 146, 146, 146, 68},
    {148, 148, 148, 148, 111},
    {153, 153, 153, 153, 143},
    {154, 154, 154, 154, 144},
    {0, 0, 0, 0, 0}
};

static const uint16_t kFloorLegendYendor3Page3[2][5] = {
    {0, 0, 0, 0, 0},
    {1, 1, 0, 10, 6}
};

static const struct { const uint16_t (*rows)[5]; unsigned count; } kFloorPagesYendor3[] = {
    {kFloorLegendYendor3Page0, 12},
    {kFloorLegendYendor3Page1, 72},
    {kFloorLegendYendor3Page2, 55},
    {kFloorLegendYendor3Page3, 2}
};

bool worldMapWallLegend(GameKind game, uint16_t wallType, const uint16_t **words) {
    if (game != GameYendor3) {
        if (wallType >= WorldMapWallTypeCountYendor2) {
            return false;
        }
        *words = kWallLegendYendor2[wallType];
        return true;
    }
    unsigned page = wallType / 100, index = wallType % 100;
    if (page >= sizeof(kWallPagesYendor3) / sizeof(kWallPagesYendor3[0]) || index >= kWallPagesYendor3[page].count) {
        return false;
    }
    *words = kWallPagesYendor3[page].rows[index];
    return true;
}

bool worldMapFloorLegend(GameKind game, uint16_t floorType, const uint16_t **words) {
    if (game != GameYendor3) {
        if (floorType >= WorldMapFloorTypeCountYendor2) {
            return false;
        }
        *words = kFloorLegendYendor2[floorType];
        return true;
    }
    unsigned page = floorType / 100, index = floorType % 100;
    if (page >= sizeof(kFloorPagesYendor3) / sizeof(kFloorPagesYendor3[0]) || index >= kFloorPagesYendor3[page].count) {
        return false;
    }
    *words = kFloorPagesYendor3[page].rows[index];
    return true;
}

bool worldMapWallPictureOffset(GameKind game, uint16_t wallType, uint16_t *outOffset) {
    const uint16_t *words;
    if (!worldMapWallLegend(game, wallType, &words)) {
        return false;
    }
    *outOffset = words[5];
    return true;
}

bool worldMapFloorPictureOffset(GameKind game, uint16_t floorType, uint16_t *outOffset) {
    const uint16_t *words;
    if (!worldMapFloorLegend(game, floorType, &words)) {
        return false;
    }
    *outOffset = words[4];
    return true;
}

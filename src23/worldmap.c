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

/*
 * Chapter 2 only (see worldmap.h): the picture-offset field (+0xA of a
 * 12-byte 0xE551 entry / +8 of a 10-byte 0xE175 entry), extracted with
 * yendor2/ida_scripts/dump_tile_tables.py. The tables' other fields aren't
 * confirmed (file-formats.md), so only this one is reproduced here.
 */
static const uint16_t g_wallPictureOffsetYendor2[WorldMapWallTypeCountYendor2] = {
    0x0016, 0x0017, 0x0018, 0x001B, 0x001E, 0x0020, 0x0022, 0x0018, 0x0022, 0x001B, 0x0050, 0x0022, 0x0018, 0x001B,
    0x001E, 0x0020, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x0029, 0x002A, 0x002A, 0x002C, 0x002D, 0x0025, 0x0026,
    0x002A, 0x002A, 0x0029, 0x0029, 0x002C, 0x002D, 0x002A, 0x002A, 0x0029, 0x0029, 0x0047, 0x0047, 0x0047, 0x0047,
    0x0047, 0x0047, 0x0025, 0x0026, 0x002A, 0x002A, 0x002C, 0x002D, 0x0025, 0x0026, 0x0024, 0x0024, 0x000F, 0x0010,
    0x000F, 0x0010,
};

static const uint16_t g_floorPictureOffsetYendor2[WorldMapFloorTypeCountYendor2] = {
    0x0000, 0x0031, 0x0040, 0x0040, 0x0040, 0x0040, 0x0041, 0x0041, 0x0041, 0x0041, 0x0042, 0x0042, 0x0039, 0x003A,
    0x003B, 0x003C, 0x003D, 0x0051, 0x0052, 0x0053, 0x0054, 0x0021, 0x0021, 0x0019, 0x001A, 0x0022, 0x002B, 0x001C,
    0x001D, 0x002B, 0x001F, 0x0023, 0x002B, 0x0055, 0x0034, 0x0037, 0x002E, 0x002F, 0x0030, 0x0032, 0x0033, 0x0035,
    0x0036, 0x0038, 0x003E, 0x003F, 0x003F, 0x0043, 0x0044, 0x0045, 0x0046, 0x0056, 0x0056, 0x0056, 0x0056, 0x0057,
    0x0056, 0x0056, 0x0057, 0x0056, 0x0056, 0x0056, 0x0058, 0x0054, 0x0052, 0x0000, 0x0000, 0x0000,
};

bool worldMapWallPictureOffset(GameKind game, uint16_t wallType, uint16_t *outOffset) {
    if (game != GameYendor2 || wallType >= WorldMapWallTypeCountYendor2) {
        return false;
    }
    *outOffset = g_wallPictureOffsetYendor2[wallType];
    return true;
}

bool worldMapFloorPictureOffset(GameKind game, uint16_t floorType, uint16_t *outOffset) {
    if (game != GameYendor2 || floorType >= WorldMapFloorTypeCountYendor2) {
        return false;
    }
    *outOffset = g_floorPictureOffsetYendor2[floorType];
    return true;
}

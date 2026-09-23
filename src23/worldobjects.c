#include "worldobjects.h"

#include <string.h>

#include "movement.h"

static const WorldObjectTableLayout g_layoutYendor2 = {0x1A1141};
static const WorldObjectTableLayout g_layoutYendor3 = {0x41090D};

const WorldObjectTableLayout *worldObjectTableLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool worldObjectTableParse(WorldObjectTable *table, GameKind game, const uint8_t *region, size_t size) {
    if (size < WorldObjectTableSize) {
        return false;
    }
    table->game = game;
    memcpy(table->data, region, WorldObjectTableSize);
    return true;
}

bool worldObjectTableParseWorldDat(WorldObjectTable *table, GameKind game, const uint8_t *worldDat, size_t size) {
    const WorldObjectTableLayout *layout = worldObjectTableLayout(game);
    if (!layout) {
        return false;
    }
    size_t needed = (size_t)layout->offset + WorldObjectTableSize;
    if (size < needed) {
        return false;
    }
    return worldObjectTableParse(table, game, worldDat + layout->offset, size - layout->offset);
}

static uint16_t readU16(const uint8_t *data, size_t offset) {
    return (uint16_t)(data[offset] | (data[offset + 1] << 8));
}

bool worldObjectFind(const WorldObjectTable *table, GameKind game, int worldCol, int worldRow,
                      WorldObjectRecord *out) {
    if (!movementInBounds(game, (uint16_t)worldCol, (uint16_t)worldRow)) {
        return false;
    }
    unsigned colIndex = (unsigned)(worldCol - WorldObjectColumnMin);
    size_t p = readU16(table->data, (size_t)colIndex * 2);

    while (p + 6 <= WorldObjectTableSize) {
        uint16_t y = readU16(table->data, p);
        if (y == 0xFFFF || worldRow < y) {
            return false; /* sorted ascending; past this point nothing can match */
        }
        if (worldRow == y) {
            out->y = y;
            out->flags = readU16(table->data, p + 2);
            out->value = readU16(table->data, p + 4);
            return true;
        }
        p += 6;
    }
    return false;
}

#include "worldmap_stdio.h"

#include <stdio.h>

bool worldMapReadWorldDatFile(WorldMap *map, GameKind game, const char *path) {
    const WorldMapLayout *layout = worldMapLayout(game);
    static uint8_t region[WorldMapRowsMax * WorldMapRowSize];
    size_t needed = layout ? (size_t)layout->rowCount * WorldMapRowSize : 0;
    if (!layout || needed > sizeof(region)) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fseek(file, (long)layout->offset, SEEK_SET) == 0 && fread(region, 1, needed, file) == needed;
    fclose(file);
    return ok && worldMapParse(map, game, region, needed);
}

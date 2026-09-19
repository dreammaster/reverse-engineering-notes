#include "monster_stdio.h"

#include <stdio.h>

bool monsterCatalogReadWorldDatFile(MonsterCatalog *catalog, GameKind game, const char *path) {
    const MonsterCatalogLayout *layout = monsterCatalogLayout(game);
    static uint8_t region[16 * 1024];
    if (!layout || layout->totalSize > sizeof(region)) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fseek(file, (long)layout->blocksOffset, SEEK_SET) == 0 &&
              fread(region, 1, layout->totalSize, file) == layout->totalSize;
    fclose(file);
    return ok && monsterCatalogParse(catalog, game, region, layout->totalSize);
}

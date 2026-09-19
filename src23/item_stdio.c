#include "item_stdio.h"

#include <stdio.h>

bool itemCatalogReadWorldDatFile(ItemCatalog *catalog, GameKind game, const char *path) {
    const ItemCatalogLayout *layout = itemCatalogLayout(game);
    static uint8_t region[64 * 1024];
    if (!layout || layout->totalSize > sizeof(region)) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fseek(file, (long)layout->itemsOffset, SEEK_SET) == 0 &&
              fread(region, 1, layout->totalSize, file) == layout->totalSize;
    fclose(file);
    return ok && itemCatalogParse(catalog, game, region, layout->totalSize);
}

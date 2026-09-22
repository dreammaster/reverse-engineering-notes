#include "document_stdio.h"

#include <stdio.h>

bool documentCatalogReadWorldDatFile(DocumentCatalog *catalog, GameKind game, const char *path) {
    const DocumentCatalogLayout *layout = documentCatalogLayout(game);
    static uint8_t region[DocumentRegionSizeMax];
    if (!layout || layout->regionSize > sizeof(region)) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fseek(file, (long)layout->regionOffset, SEEK_SET) == 0 &&
              fread(region, 1, layout->regionSize, file) == layout->regionSize;
    fclose(file);
    return ok && documentCatalogParse(catalog, game, region, layout->regionSize);
}

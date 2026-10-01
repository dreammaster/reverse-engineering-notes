#include "spellrecord_stdio.h"

#include <stdio.h>

bool spellCatalogReadWorldDatFile(SpellCatalog *catalog, GameKind game, const char *path) {
    const SpellCatalogLayout *layout = spellCatalogLayout(game);
    static uint8_t region[SpellRecordCountMax * SpellRecordSize];
    size_t totalSize = (size_t)layout->recordCount * SpellRecordSize;
    if (!layout || totalSize > sizeof(region)) {
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fseek(file, (long)layout->recordsOffset, SEEK_SET) == 0 &&
              fread(region, 1, totalSize, file) == totalSize;
    fclose(file);
    return ok && spellCatalogParse(catalog, game, region, totalSize);
}

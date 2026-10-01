#include "spellrecord.h"

#include <string.h>

static const SpellCatalogLayout g_layoutYendor2 = {0x1AA5DD, SpellRecordCountYendor2};
static const SpellCatalogLayout g_layoutYendor3 = {0x41B5BF, SpellRecordCountYendor3};

const SpellCatalogLayout *spellCatalogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool spellCatalogParse(SpellCatalog *catalog, GameKind game, const uint8_t *region, size_t size) {
    const SpellCatalogLayout *layout = spellCatalogLayout(game);
    if (!layout || size < (size_t)layout->recordCount * SpellRecordSize) {
        return false;
    }
    memset(catalog, 0, sizeof(*catalog));
    catalog->game = game;
    catalog->recordCount = layout->recordCount;
    memcpy(catalog->records, region, (size_t)layout->recordCount * SpellRecordSize);
    return true;
}

bool spellCatalogParseWorldDat(SpellCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const SpellCatalogLayout *layout = spellCatalogLayout(game);
    if (!layout || size < (size_t)layout->recordsOffset + (size_t)layout->recordCount * SpellRecordSize) {
        return false;
    }
    return spellCatalogParse(catalog, game, worldDat + layout->recordsOffset, (size_t)layout->recordCount * SpellRecordSize);
}

const uint8_t *spellRecord(const SpellCatalog *catalog, unsigned id) {
    if (id == 0 || id > catalog->recordCount) {
        return NULL;
    }
    return catalog->records + (size_t)(id - 1) * SpellRecordSize;
}

uint16_t spellGetU16(const uint8_t *record, unsigned offset) {
    return (uint16_t)(record[offset] | (record[offset + 1] << 8));
}

void spellGetName(const uint8_t *record, char *out) {
    unsigned len = SpellNameFieldSize;
    while (len > 0 && (record[len - 1] == 0 || record[len - 1] == ' ')) {
        len--;
    }
    memcpy(out, record, len);
    out[len] = '\0';
}

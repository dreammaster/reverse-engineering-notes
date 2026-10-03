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

SpellBranch spellSelectBranch(const uint8_t *record) {
    static const struct {
        uint16_t bit;
        SpellBranch branch;
    } flagsB[] = {
        {0x8000, SpellBranchSingleTarget},      {0x4000, SpellBranchWholeParty},
        {0x0080, SpellBranchLightTimer},        {0x0010, SpellBranchHeldItem},
        {0x0004, SpellBranchTeleportEngage},    {0x0020, SpellBranchLocationBookmark},
        {0x0008, SpellBranchTriggerCurgameEvent}, {0x0002, SpellBranchRest},
        {0x0001, SpellBranchKnock},             {0x0040, SpellBranchFacingLockOrEvent},
        {0x2000, SpellBranchAttackActiveMonster}, {0x1000, SpellBranchAttackAllSlots},
        {0x0100, SpellBranchProjectile},        {0x0200, SpellBranchScreenWide},
    };
    uint16_t b = spellGetU16(record, SpellFieldFlagsB);
    for (unsigned i = 0; i < sizeof(flagsB) / sizeof(flagsB[0]); i++) {
        if (b & flagsB[i].bit) {
            if (flagsB[i].branch == SpellBranchAttackActiveMonster &&
                (spellGetU16(record, SpellFieldResistFlags) & (SpellResistLifeForceCaster | SpellResistLifeForceParty))) {
                return SpellBranchLifeForce;
            }
            return flagsB[i].branch;
        }
    }
    uint16_t r = spellGetU16(record, SpellFieldResistFlags);
    if (r & 0x8) {
        return SpellBranchBeam;
    }
    if (r & 0x2) {
        return SpellBranchTremor;
    }
    if (r & 0x4) {
        return SpellBranchRain;
    }
    if (r & 0x1) {
        return SpellBranchTurbulence;
    }
    return SpellBranchNone;
}

uint16_t spellCreatedItemRollBound(const uint8_t *record) {
    if (spellGetU16(record, SpellFieldAttackMagnitude) != 0) {
        return 0;
    }
    return (uint16_t)(spellGetU16(record, SpellFieldCreatedItemMax) - spellGetU16(record, SpellFieldCreatedItemMin));
}

SpellCreatedItem spellCreatedItem(const uint8_t *record, uint16_t roll) {
    SpellCreatedItem item;
    uint16_t fixed = spellGetU16(record, SpellFieldAttackMagnitude);
    item.itemId = fixed != 0 ? fixed : (uint16_t)(roll + spellGetU16(record, SpellFieldCreatedItemMin));
    item.extra = spellGetU16(record, SpellFieldCreatedItemExtra);
    return item;
}

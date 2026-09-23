#include "monster.h"

#include <string.h>

/*
 * From the record-setup stubs WorldDat_setBlock5 (106-byte stat blocks) and
 * WorldDat_setBlock6 (u16 type-to-block lookup); the lookup follows the last
 * block in both games. Chapter 2's lookup fills its whole 5000-byte region
 * (2500 entries). Chapter 3's real entries end at type id 1862, the highest id
 * in its flag table; the bytes after that are unrelated engine constants.
 */
static const MonsterCatalogLayout g_layoutYendor2 = {
    0x1A78A9, 62, 2500, 62 * MonsterBlockSize + 2500 * 2,
};

static const MonsterCatalogLayout g_layoutYendor3 = {
    0x417075, 73, 1863, 73 * MonsterBlockSize + 1863 * 2,
};

typedef struct {
    uint16_t typeId;
    int16_t flagA;
    int16_t flagB;
} DeathFlagEntry;

/* Ascending by type id; extracted from SW.EXE (DS:0xE4E9) and Yendor3-full.exe (DS:0xCE51). */
static const DeathFlagEntry g_deathFlagsYendor2[] = {
    {15, 14, 0},    {207, 25, 0},   {275, 22, 0},   {376, 147, 0},  {780, 48, 0},   {929, 116, 0},
    {990, 133, 0},  {1099, 78, 0},  {1216, 86, 0},  {1261, 83, 0},  {1309, 92, 0},  {1758, 105, 0},
    {1759, 106, 0}, {1760, 107, 0}, {1786, 42, 0},  {1819, 66, 0},  {1959, 143, 0},
};

static const DeathFlagEntry g_deathFlagsYendor3[] = {
    {1, 1, 0},      {10, 6, 0},     {39, 9, 0},     {145, 20, 0},   {203, 34, 0},   {241, 41, 0},
    {310, 52, 0},   {314, 56, 0},   {362, 59, 0},   {444, 68, 0},   {487, 80, 0},   {806, 99, 0},
    {849, 103, 0},  {900, 108, 0},  {992, 114, 0},  {1154, 162, 0}, {1181, 163, 0}, {1243, 183, 0},
    {1422, 201, 0}, {1566, 206, 0}, {1750, 211, 0}, {1861, 223, 0}, {1862, 224, 0},
};

const MonsterCatalogLayout *monsterCatalogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool monsterCatalogParse(MonsterCatalog *catalog, GameKind game, const uint8_t *region, size_t size) {
    const MonsterCatalogLayout *layout = monsterCatalogLayout(game);
    if (!layout || size < layout->totalSize) {
        return false;
    }
    memset(catalog, 0, sizeof(*catalog));
    catalog->game = game;
    catalog->blockCount = layout->blockCount;
    catalog->lookupCount = layout->lookupCount;
    size_t blockBytes = (size_t)layout->blockCount * MonsterBlockSize;
    memcpy(catalog->blocks, region, blockBytes);
    memcpy(catalog->lookup, region + blockBytes, (size_t)layout->lookupCount * 2);
    return true;
}

bool monsterCatalogParseWorldDat(MonsterCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const MonsterCatalogLayout *layout = monsterCatalogLayout(game);
    if (!layout || size < (size_t)layout->blocksOffset + layout->totalSize) {
        return false;
    }
    return monsterCatalogParse(catalog, game, worldDat + layout->blocksOffset, layout->totalSize);
}

const uint8_t *monsterCatalogBlock(const MonsterCatalog *catalog, unsigned index) {
    if (index >= catalog->blockCount) {
        return NULL;
    }
    return catalog->blocks + (size_t)index * MonsterBlockSize;
}

unsigned monsterCatalogBlockIndex(const MonsterCatalog *catalog, unsigned typeId) {
    if (typeId >= catalog->lookupCount) {
        return 0;
    }
    unsigned index = catalog->lookup[typeId * 2] | (catalog->lookup[typeId * 2 + 1] << 8);
    return index < catalog->blockCount ? index : 0;
}

uint16_t monsterGetU16(const uint8_t *record, unsigned offset) {
    return (uint16_t)(record[offset] | (record[offset + 1] << 8));
}

void monsterSetU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

bool monsterDeathFlags(GameKind game, unsigned typeId, int16_t *flagA, int16_t *flagB) {
    const DeathFlagEntry *table = game == GameYendor2 ? g_deathFlagsYendor2 : g_deathFlagsYendor3;
    size_t count = game == GameYendor2 ? sizeof(g_deathFlagsYendor2) / sizeof(g_deathFlagsYendor2[0])
                                       : sizeof(g_deathFlagsYendor3) / sizeof(g_deathFlagsYendor3[0]);
    for (size_t i = 0; i < count; i++) {
        if (table[i].typeId == typeId) {
            *flagA = table[i].flagA;
            *flagB = table[i].flagB;
            return true;
        }
        if (table[i].typeId > typeId) {
            break;
        }
    }
    return false;
}

unsigned monsterAmbushThreshold(uint16_t awareness) {
    if (awareness & MonsterAmbushChanceVeryHigh) return 90;
    if (awareness & MonsterAmbushChanceHigh) return 75;
    if (awareness & MonsterAmbushChanceMedium) return 50;
    if (awareness & MonsterAmbushChanceLow) return 25;
    return 5;
}

bool monsterRecordSpawn(uint8_t *record, const MonsterCatalog *catalog, unsigned typeId) {
    unsigned index = monsterCatalogBlockIndex(catalog, typeId);
    if (index == 0) {
        return false;
    }
    memset(record, 0, MonsterRecordSize);
    monsterSetU16(record, MonsterFieldType, (uint16_t)typeId);
    memcpy(record + MonsterBlockOffset, monsterCatalogBlock(catalog, index), MonsterBlockSize);
    monsterSetU16(record, MonsterFieldHealth, monsterGetU16(record, MonsterFieldMaxHealth));

    int16_t flagA;
    int16_t flagB;
    if (monsterDeathFlags(catalog->game, typeId, &flagA, &flagB)) {
        monsterSetU16(record, MonsterFieldFlagOnDeath, (uint16_t)flagA);
        monsterSetU16(record, MonsterFieldFlagOnDeath2, (uint16_t)flagB);
    }
    return true;
}

void monsterRecordPlace(uint8_t *record, uint16_t x, uint16_t y, uint16_t gridOriginRow, uint16_t gridOriginCol) {
    monsterSetU16(record, MonsterFieldWorldX, x);
    monsterSetU16(record, MonsterFieldWorldY, y);
    monsterSetU16(record, MonsterFieldCell, (uint16_t)((uint16_t)(y - gridOriginRow) * 0x270 + (uint16_t)(x - gridOriginCol) * 8));
}

void monsterRecordStartAnimation(uint8_t *record, unsigned randomExtra) {
    monsterSetU16(record, MonsterFieldAnim, (uint16_t)(monsterGetU16(record, MonsterFieldSpriteBase) + randomExtra));
    monsterSetU16(record, MonsterFieldAnimSet,
                  (monsterGetU16(record, MonsterFieldFlags) & MonsterFlagAltSprite) ? 0x0A : 0x0D);
}

const uint8_t *monsterLoot(const uint8_t *record, MonsterLoot kind) {
    switch (kind) {
    case MonsterLootGold:
        return record + MonsterFieldLootGold;
    case MonsterLootNuore:
        return record + MonsterFieldLootNuore;
    case MonsterLootOre:
        return record + MonsterFieldLootOre;
    case MonsterLootExperience:
        return record + MonsterFieldExperience;
    }
    return NULL;
}

bool monsterIsImmune(const uint8_t *record, MonsterImmunity kind) {
    return (monsterGetU16(record, MonsterFieldImmunities) & kind) != 0;
}

bool monsterResistsMagic(const uint8_t *record) {
    return (monsterGetU16(record, MonsterFieldResistances) & MonsterResistMagicMask) != 0;
}

bool monsterResistsPhysical(const uint8_t *record) {
    return (monsterGetU16(record, MonsterFieldResistances) & MonsterResistPhysicalMask) != 0;
}

static size_t lineLength(const uint8_t *record, unsigned offset) {
    size_t length = 0;
    while (length < MonsterNameLineSize - 1 && record[offset + length] != 0) {
        length++;
    }
    while (length > 0 && record[offset + length - 1] == ' ') {
        length--;
    }
    return length;
}

void monsterGetNameLine(const uint8_t *record, unsigned line, char out[MonsterNameLineSize]) {
    if (line >= 2) {
        out[0] = '\0';
        return;
    }
    unsigned offset = line == 0 ? MonsterFieldName1 : MonsterFieldName2;
    size_t length = lineLength(record, offset);
    memcpy(out, record + offset, length);
    out[length] = '\0';
}

void monsterGetName(const uint8_t *record, char out[MonsterNameBufferSize]) {
    size_t first = lineLength(record, MonsterFieldName1);
    size_t second = lineLength(record, MonsterFieldName2);
    memcpy(out, record + MonsterFieldName1, first);
    out[first] = ' ';
    memcpy(out + first + 1, record + MonsterFieldName2, second);
    out[first + 1 + second] = '\0';
}

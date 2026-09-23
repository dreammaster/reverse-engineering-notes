#include "lockcatalog.h"

#include <string.h>

static const LockCatalogLayout g_layoutYendor2 = {0x7E5BA, LockRecordCountYendor2};
static const LockCatalogLayout g_layoutYendor3 = {0x8F00A, LockRecordCountYendor3};

const LockCatalogLayout *lockCatalogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool lockCatalogParse(LockCatalog *catalog, GameKind game, const uint8_t *region, size_t size) {
    const LockCatalogLayout *layout = lockCatalogLayout(game);
    if (!layout || size < (size_t)layout->recordCount * LockRecordSize) {
        return false;
    }
    catalog->game = game;
    catalog->recordCount = layout->recordCount;
    memcpy(catalog->records, region, (size_t)layout->recordCount * LockRecordSize);
    return true;
}

bool lockCatalogParseWorldDat(LockCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const LockCatalogLayout *layout = lockCatalogLayout(game);
    if (!layout) {
        return false;
    }
    size_t needed = (size_t)layout->offset + (size_t)layout->recordCount * LockRecordSize;
    if (size < needed) {
        return false;
    }
    return lockCatalogParse(catalog, game, worldDat + layout->offset, size - layout->offset);
}

static uint16_t readU16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

bool lockCatalogRecord(const LockCatalog *catalog, unsigned lockId, LockRecord *out) {
    if (lockId == 0 || lockId > catalog->recordCount) {
        return false;
    }
    const uint8_t *rec = catalog->records + (size_t)(lockId - 1) * LockRecordSize;
    out->flags = readU16(rec);
    out->price = readU16(rec + 2);
    memcpy(out->rest, rec + 4, sizeof(out->rest));
    return true;
}

/* Matches ShowLockStatus's own test order (yendor2.asm:12776 on). */
uint16_t lockRequiredKeyType(uint16_t flags) {
    static const uint16_t kOrder[] = {LockFlagKeyBrass,  LockFlagKeyBronze, LockFlagKeyCopper, LockFlagKeyIron,
                                       LockFlagKeySteel,  LockFlagKeySilver, LockFlagKeyGold};
    for (size_t i = 0; i < sizeof(kOrder) / sizeof(kOrder[0]); i++) {
        if (flags & kOrder[i]) {
            return kOrder[i];
        }
    }
    return 0;
}

const char *lockKeyTypeName(uint16_t keyFlag) {
    switch (keyFlag) {
    case LockFlagKeyBrass: return "BRASS KEY";
    case LockFlagKeyBronze: return "BRONZE KEY";
    case LockFlagKeyCopper: return "COPPER KEY";
    case LockFlagKeyIron: return "IRON KEY";
    case LockFlagKeySteel: return "STEEL KEY";
    case LockFlagKeySilver: return "SILVER KEY";
    case LockFlagKeyGold: return "GOLD KEY";
    default: return NULL;
    }
}

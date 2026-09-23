/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_lockcatalog test_lockcatalog.c ../lockcatalog.c && ./test_lockcatalog
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lockcatalog.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static void checkStr(const char *label, const char *actual, const char *expected) {
    bool ok = (actual == NULL && expected == NULL) || (actual && expected && strcmp(actual, expected) == 0);
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %s, want %s\n", label, actual ? actual : "(null)", expected ? expected : "(null)");
    }
}

static void testLayout(void) {
    checkU32("yendor2 offset", lockCatalogLayout(GameYendor2)->offset, 0x7E5BA);
    checkU32("yendor2 record count", lockCatalogLayout(GameYendor2)->recordCount, 608);
    checkU32("yendor3 offset", lockCatalogLayout(GameYendor3)->offset, 0x8F00A);
    checkU32("yendor3 record count", lockCatalogLayout(GameYendor3)->recordCount, 1008);
}

static uint8_t g_region[LockRecordCountMax * LockRecordSize];
static LockCatalog g_catalog;

static void testParse(void) {
    size_t needed = (size_t)LockRecordCountYendor2 * LockRecordSize;
    for (size_t i = 0; i < needed; i++) {
        g_region[i] = (uint8_t)(i * 5 + 3);
    }
    check("a region one byte short is rejected", !lockCatalogParse(&g_catalog, GameYendor2, g_region, needed - 1));
    check("an exact region parses", lockCatalogParse(&g_catalog, GameYendor2, g_region, needed));

    LockRecord rec;
    check("lock id 0 is invalid (1-based)", !lockCatalogRecord(&g_catalog, 0, &rec));
    check("lock id 609 is out of range for yendor2", !lockCatalogRecord(&g_catalog, 609, &rec));
    check("lock id 1 reads the region's first bytes",
          lockCatalogRecord(&g_catalog, 1, &rec) && rec.flags == (uint16_t)(g_region[0] | (g_region[1] << 8)) &&
              rec.price == (uint16_t)(g_region[2] | (g_region[3] << 8)));
    check("lock id 608 (last) reads the region's last record",
          lockCatalogRecord(&g_catalog, 608, &rec) &&
              rec.flags == (uint16_t)(g_region[607 * 26] | (g_region[607 * 26 + 1] << 8)));

    static uint8_t world[0x7E5BA + LockRecordCountYendor2 * LockRecordSize + 16];
    memcpy(world + 0x7E5BA, g_region, needed);
    check("whole-file parse finds the region at the yendor2 offset",
          lockCatalogParseWorldDat(&g_catalog, GameYendor2, world, sizeof(world)));
    check("a WORLD.DAT shorter than offset+region is rejected",
          !lockCatalogParseWorldDat(&g_catalog, GameYendor2, world, 0x7E5BA + needed - 1));
}

static void testKeyType(void) {
    checkU32("brass wins when only brass is set", lockRequiredKeyType(LockFlagKeyBrass), LockFlagKeyBrass);
    checkU32("gold alone", lockRequiredKeyType(LockFlagKeyGold), LockFlagKeyGold);
    checkU32("brass beats gold when both set (brass tested first)",
             lockRequiredKeyType(LockFlagKeyBrass | LockFlagKeyGold), LockFlagKeyBrass);
    checkU32("steel beats silver and gold", lockRequiredKeyType(LockFlagKeySteel | LockFlagKeySilver | LockFlagKeyGold),
             LockFlagKeySteel);
    checkU32("no key bits set returns 0", lockRequiredKeyType(LockFlagMagical), 0);
    checkU32("no bits at all returns 0", lockRequiredKeyType(0), 0);

    checkStr("brass name", lockKeyTypeName(LockFlagKeyBrass), "BRASS KEY");
    checkStr("bronze name", lockKeyTypeName(LockFlagKeyBronze), "BRONZE KEY");
    checkStr("copper name", lockKeyTypeName(LockFlagKeyCopper), "COPPER KEY");
    checkStr("iron name", lockKeyTypeName(LockFlagKeyIron), "IRON KEY");
    checkStr("steel name", lockKeyTypeName(LockFlagKeySteel), "STEEL KEY");
    checkStr("silver name", lockKeyTypeName(LockFlagKeySilver), "SILVER KEY");
    checkStr("gold name", lockKeyTypeName(LockFlagKeyGold), "GOLD KEY");
    checkStr("an unrecognized value has no name", lockKeyTypeName(0), NULL);
    checkStr("magical bit alone has no key name", lockKeyTypeName(LockFlagMagical), NULL);
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir, LockCatalog *outCatalog) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return false;
    }
    uint8_t *buf = malloc((size_t)size);
    bool ok = buf && fread(buf, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    if (ok) {
        ok = lockCatalogParseWorldDat(outCatalog, game, buf, (size_t)size);
    }
    free(buf);
    return ok;
}

typedef struct {
    unsigned magicalCount;
    unsigned multiKeyBitCount;
    unsigned keyCounts[7]; /* indexed to match kKeyBits below */
} KeyTally;

static const uint16_t kKeyBits[7] = {LockFlagKeyBrass,  LockFlagKeyBronze, LockFlagKeyCopper, LockFlagKeyIron,
                                      LockFlagKeySteel,  LockFlagKeySilver, LockFlagKeyGold};

static KeyTally tallyCatalog(const LockCatalog *catalog) {
    KeyTally tally = {0, 0, {0, 0, 0, 0, 0, 0, 0}};
    for (unsigned id = 1; id <= catalog->recordCount; id++) {
        LockRecord rec;
        if (!lockCatalogRecord(catalog, id, &rec)) {
            continue;
        }
        if (rec.flags & LockFlagMagical) {
            tally.magicalCount++;
        }
        unsigned setCount = 0;
        for (int i = 0; i < 7; i++) {
            if (rec.flags & kKeyBits[i]) {
                setCount++;
            }
        }
        if (setCount > 1) {
            tally.multiKeyBitCount++;
        } else if (setCount == 1) {
            for (int i = 0; i < 7; i++) {
                if (rec.flags & kKeyBits[i]) {
                    tally.keyCounts[i]++;
                }
            }
        }
    }
    return tally;
}

static void testRealYendor2(void) {
    static LockCatalog catalog;
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", &catalog)) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    LockRecord rec;
    check("yendor2 lock id 1 parses", lockCatalogRecord(&catalog, 1, &rec));
    checkU32("yendor2 lock id 1 flags", rec.flags, 0x0041);
    checkU32("yendor2 lock id 1 price", rec.price, 0);

    KeyTally tally = tallyCatalog(&catalog);
    checkU32("yendor2 magical locks", tally.magicalCount, 17);
    checkU32("yendor2 multi-key-bit records", tally.multiKeyBitCount, 0);
    checkU32("yendor2 brass-key locks", tally.keyCounts[0], 1);
    checkU32("yendor2 bronze-key locks", tally.keyCounts[1], 5);
    checkU32("yendor2 copper-key locks", tally.keyCounts[2], 4);
    checkU32("yendor2 iron-key locks", tally.keyCounts[3], 2);
    checkU32("yendor2 steel-key locks", tally.keyCounts[4], 2);
    checkU32("yendor2 silver-key locks", tally.keyCounts[5], 3);
    checkU32("yendor2 gold-key locks", tally.keyCounts[6], 9);
}

static void testRealYendor3(void) {
    static LockCatalog catalog;
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", &catalog)) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    LockRecord rec;
    check("yendor3 lock id 1 parses", lockCatalogRecord(&catalog, 1, &rec));
    checkU32("yendor3 lock id 1 flags", rec.flags, 0x0081);

    KeyTally tally = tallyCatalog(&catalog);
    checkU32("yendor3 magical locks", tally.magicalCount, 214);
    check("yendor3 has real multi-key-bit records (unlike yendor2)", tally.multiKeyBitCount == 123);
    checkU32("yendor3 brass-key locks", tally.keyCounts[0], 6);
    checkU32("yendor3 bronze-key locks", tally.keyCounts[1], 16);
    checkU32("yendor3 copper-key locks", tally.keyCounts[2], 178);
    checkU32("yendor3 iron-key locks", tally.keyCounts[3], 8);
    checkU32("yendor3 steel-key locks", tally.keyCounts[4], 4);
    checkU32("yendor3 silver-key locks", tally.keyCounts[5], 11);
    checkU32("yendor3 gold-key locks", tally.keyCounts[6], 14);
}

int main(void) {
    testLayout();
    testParse();
    testKeyType();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

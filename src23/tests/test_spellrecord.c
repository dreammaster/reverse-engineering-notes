/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_spellrecord test_spellrecord.c ../spellrecord.c ../spellrecord_stdio.c && ./test_spellrecord
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spellrecord.h"
#include "spellrecord_stdio.h"

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void testLayouts(void) {
    const SpellCatalogLayout *y2 = spellCatalogLayout(GameYendor2);
    const SpellCatalogLayout *y3 = spellCatalogLayout(GameYendor3);
    check("yendor2 layout exists", y2 != NULL);
    check("yendor3 layout exists", y3 != NULL);
    checkU32("yendor2 offset", y2->recordsOffset, 0x1AA5DD);
    checkU32("yendor2 count", y2->recordCount, 125);
    checkU32("yendor3 offset", y3->recordsOffset, 0x41B5BF);
    checkU32("yendor3 count", y3->recordCount, 107);
}

static void testParseAndBounds(void) {
    uint8_t region[SpellRecordCountYendor2 * SpellRecordSize];
    memset(region, 0, sizeof(region));
    region[0] = 'H';
    region[(SpellRecordCountYendor2 - 1) * SpellRecordSize] = 'Z';

    SpellCatalog catalog;
    check("a too-small region is rejected", !spellCatalogParse(&catalog, GameYendor2, region, sizeof(region) - 1));
    check("an exact-size region parses", spellCatalogParse(&catalog, GameYendor2, region, sizeof(region)));

    check("id 0 is out of range", spellRecord(&catalog, 0) == NULL);
    check("id 1 is the first record", spellRecord(&catalog, 1)[0] == 'H');
    check("the last id is the last record", spellRecord(&catalog, 125)[0] == 'Z');
    check("one past the last id is out of range", spellRecord(&catalog, 126) == NULL);
}

static void testFieldAccess(void) {
    uint8_t record[SpellRecordSize];
    memset(record, 0, sizeof(record));
    memcpy(record + SpellFieldName, "COLD SLASH", 10);
    record[SpellFieldMpCost] = 6;
    record[SpellFieldMpCost + 1] = 0;
    record[SpellFieldResistFlags] = 0;
    record[SpellFieldResistFlags + 1] = 0x20; /* 0x2000 */

    char name[SpellNameFieldSize + 1];
    spellGetName(record, name);
    check("name is trimmed of trailing spaces/NULs", strcmp(name, "COLD SLASH") == 0);
    checkU32("field access round-trips a u16", spellGetU16(record, SpellFieldMpCost), 6);
    checkU32("field access reads the high byte too", spellGetU16(record, SpellFieldResistFlags), 0x2000);
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir, SpellCatalog *catalog) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return spellCatalogReadWorldDatFile(catalog, game, path);
}

static bool recordNameIs(const SpellCatalog *catalog, unsigned id, const char *expected) {
    char name[SpellNameFieldSize + 1];
    const uint8_t *record = spellRecord(catalog, id);
    if (!record) {
        return false;
    }
    spellGetName(record, name);
    return strcmp(name, expected) == 0;
}

static void checkInvariants(const char *game, const SpellCatalog *catalog) {
    char label[96];
    bool costsOk = true;
    bool printableOk = true;
    unsigned attackPathCount = 0;
    unsigned clearAwareCount = 0;

    for (unsigned id = 1; id <= catalog->recordCount; id++) {
        const uint8_t *record = spellRecord(catalog, id);
        char name[SpellNameFieldSize + 1];
        spellGetName(record, name);
        for (const char *c = name; *c; c++) {
            if (*c < ' ' || *c > '~') {
                printableOk = false;
            }
        }
        /* Costs are plain binary counters here (not packed BCD) -- sanity-bound them generously. */
        if (spellGetU16(record, SpellFieldMpCost) > 999 || spellGetU16(record, SpellFieldNuoreCost) > 999 ||
            spellGetU16(record, SpellFieldOreCost) > 999) {
            costsOk = false;
        }
        if (spellGetU16(record, SpellFieldFlagsB) & SpellFlagsBAttackPath) {
            attackPathCount++;
        }
        if (spellGetU16(record, SpellFieldResistFlags) & SpellResistClearAware) {
            clearAwareCount++;
        }
    }

    snprintf(label, sizeof(label), "%s: every record name is printable text", game);
    check(label, printableOk);
    snprintf(label, sizeof(label), "%s: every cost field is a plausible small counter", game);
    check(label, costsOk);
    snprintf(label, sizeof(label), "%s: SpellResistClearAware is never set by real data", game);
    checkU32(label, clearAwareCount, 0);
    snprintf(label, sizeof(label), "%s: at least one real record uses the attack path", game);
    check(label, attackPathCount > 0);
}

static void testRealYendor2(void) {
    SpellCatalog catalog;
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", &catalog)) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    check("record 1 is HEAL", recordNameIs(&catalog, 1, "HEAL"));
    check("record 2 is MAGIC ATTACK", recordNameIs(&catalog, 2, "MAGIC ATTACK"));
    check("record 3 is SLING SHOT", recordNameIs(&catalog, 3, "SLING SHOT"));
    check("record 4 is COLD SLASH", recordNameIs(&catalog, 4, "COLD SLASH"));
    check("record 5 is MINOR WOUNDS", recordNameIs(&catalog, 5, "MINOR WOUNDS"));
    check("record 6 is MINER'S LIGHT I", recordNameIs(&catalog, 6, "MINER'S LIGHT I"));
    check("record 11 is INSECT REPELLENT", recordNameIs(&catalog, 11, "INSECT REPELLENT"));

    const uint8_t *heal = spellRecord(&catalog, 1);
    checkU32("HEAL costs 3 MP", spellGetU16(heal, SpellFieldMpCost), 3);
    checkU32("HEAL costs 2 NUORE", spellGetU16(heal, SpellFieldNuoreCost), 2);
    check("HEAL is the single-target icon-bar branch", spellGetU16(heal, SpellFieldFlagsB) & SpellFlagsBSingleTarget);

    const uint8_t *magicAttack = spellRecord(&catalog, 2);
    check("MAGIC ATTACK uses the attack path", spellGetU16(magicAttack, SpellFieldFlagsB) & SpellFlagsBAttackPath);
    checkU32("MAGIC ATTACK's own attack magnitude", spellGetU16(magicAttack, SpellFieldAttackMagnitude), 9);

    const uint8_t *insectRepellent = spellRecord(&catalog, 11);
    checkU32("INSECT REPELLENT is type-restricted to monster type 9", spellGetU16(insectRepellent, SpellFieldTargetTypeId), 9);
    check("...and the restriction bit is actually set", spellGetU16(insectRepellent, SpellFieldResistFlags) & SpellResistTypeRestricted);

    checkInvariants("yendor2", &catalog);
}

static void testRealYendor3(void) {
    SpellCatalog catalog;
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", &catalog)) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    check("record 1 is HEAL, same as chapter 2", recordNameIs(&catalog, 1, "HEAL"));
    check("record 6 is MINER'S LIGHT I, same as chapter 2", recordNameIs(&catalog, 6, "MINER'S LIGHT I"));

    checkInvariants("yendor3", &catalog);
}

int main(void) {
    testLayouts();
    testParseAndBounds();
    testFieldAccess();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

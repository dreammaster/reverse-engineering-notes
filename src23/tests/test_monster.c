/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_monster test_monster.c ../monster.c ../monster_stdio.c && ./test_monster
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "monster.h"
#include "monster_stdio.h"
#include "savegame.h"

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

static MonsterCatalog g_catalog;
static uint8_t g_region[16 * 1024];
static uint8_t g_record[MonsterRecordSize];

static void testLayouts(void) {
    checkU32("a record is 50 bytes of state plus a 106-byte block", MonsterBlockOffset + MonsterBlockSize,
             MonsterRecordSize);
    checkU32("g_levelMonsters block size in the save file", MonsterPoolSize * MonsterRecordSize, 12480);
    checkU32("record size matches the save layout", MonsterRecordSize, SaveMonsterRecordSize);
    checkU32("pool size matches the save layout", MonsterPoolSize, SaveMonsterCount);

    for (int g = 0; g < 2; g++) {
        const MonsterCatalogLayout *layout = monsterCatalogLayout((GameKind)g);
        const char *label = g == 0 ? "yendor2" : "yendor3";
        char text[80];
        snprintf(text, sizeof(text), "%s: total is blocks plus lookup", label);
        checkU32(text, layout->totalSize, layout->blockCount * MonsterBlockSize + layout->lookupCount * 2u);
        snprintf(text, sizeof(text), "%s: counts fit the catalog arrays", label);
        check(text, layout->blockCount <= MonsterBlockCountMax && layout->lookupCount <= MonsterLookupCountMax);
    }
    checkU32("yendor2 has 62 blocks (index 0 empty)", monsterCatalogLayout(GameYendor2)->blockCount, 62);
    checkU32("yendor3 has 73 blocks", monsterCatalogLayout(GameYendor3)->blockCount, 73);
    checkU32("yendor2 blocks end where its lookup starts (0x1A9255)",
             monsterCatalogLayout(GameYendor2)->blocksOffset + 62 * MonsterBlockSize, 0x1A9255);
    checkU32("yendor3 blocks end where its lookup starts (0x418EAF)",
             monsterCatalogLayout(GameYendor3)->blocksOffset + 73 * MonsterBlockSize, 0x418EAF);
}

static void testParse(void) {
    const MonsterCatalogLayout *layout = monsterCatalogLayout(GameYendor2);
    for (uint32_t i = 0; i < layout->totalSize; i++) {
        g_region[i] = (uint8_t)(i * 5 + 1);
    }
    check("a region one byte short is rejected",
          !monsterCatalogParse(&g_catalog, GameYendor2, g_region, layout->totalSize - 1));
    check("an exact region parses", monsterCatalogParse(&g_catalog, GameYendor2, g_region, layout->totalSize));
    check("block 0 is first", monsterCatalogBlock(&g_catalog, 0) == g_catalog.blocks);
    check("block 61 is the last", monsterCatalogBlock(&g_catalog, 61) == g_catalog.blocks + 61 * MonsterBlockSize);
    check("block 62 does not exist", monsterCatalogBlock(&g_catalog, 62) == NULL);
    check("the lookup follows the last block",
          memcmp(g_catalog.lookup, g_region + 62 * MonsterBlockSize, 2500 * 2) == 0);

    /* Lookup entries: type -> block index, little-endian; out-of-range indexes and types give 0. */
    memset(g_catalog.lookup, 0, sizeof(g_catalog.lookup));
    g_catalog.lookup[2 * 7] = 61;
    g_catalog.lookup[2 * 8] = 62;
    g_catalog.lookup[2 * 9] = 0xFF;
    g_catalog.lookup[2 * 9 + 1] = 0xFF;
    checkU32("type 7 maps to block 61", monsterCatalogBlockIndex(&g_catalog, 7), 61);
    checkU32("a block index past the table maps to 0", monsterCatalogBlockIndex(&g_catalog, 8), 0);
    checkU32("0xFFFF maps to 0", monsterCatalogBlockIndex(&g_catalog, 9), 0);
    checkU32("a type past the lookup maps to 0", monsterCatalogBlockIndex(&g_catalog, 2500), 0);

    static uint8_t world[0x1AE075];
    memcpy(world + layout->blocksOffset, g_region, layout->totalSize);
    check("whole-file parse finds the region",
          monsterCatalogParseWorldDat(&g_catalog, GameYendor2, world, sizeof(world)) &&
              memcmp(g_catalog.blocks, g_region, MonsterBlockSize) == 0);
    check("a WORLD.DAT that ends inside the region is rejected",
          !monsterCatalogParseWorldDat(&g_catalog, GameYendor2, world, layout->blocksOffset + layout->totalSize - 1));
}

static void testRecord(void) {
    memset(g_record, 0, sizeof(g_record));
    monsterSetU16(g_record, MonsterFieldSpriteBase, 110);
    monsterRecordStartAnimation(g_record, 3);
    checkU32("animation starts at sprite base + extra", monsterGetU16(g_record, MonsterFieldAnim), 113);
    checkU32("without the alt-sprite flag the anim set is 0xD", monsterGetU16(g_record, MonsterFieldAnimSet), 0x0D);
    monsterSetU16(g_record, MonsterFieldFlags, MonsterFlagAltSprite);
    monsterRecordStartAnimation(g_record, 0);
    checkU32("with the alt-sprite flag the anim set is 0xA", monsterGetU16(g_record, MonsterFieldAnimSet), 0x0A);

    monsterRecordPlace(g_record, 100, 50, 40, 90);
    checkU32("world x", monsterGetU16(g_record, MonsterFieldWorldX), 100);
    checkU32("world y", monsterGetU16(g_record, MonsterFieldWorldY), 50);
    checkU32("cell is (y-row)*0x270 + (x-col)*8", monsterGetU16(g_record, MonsterFieldCell), 10 * 0x270 + 10 * 8);

    memset(g_record, 0, sizeof(g_record));
    monsterSetU16(g_record, MonsterFieldImmunities, MonsterImmuneCold | MonsterImmunePoison);
    check("cold immunity", monsterIsImmune(g_record, MonsterImmuneCold));
    check("poison immunity", monsterIsImmune(g_record, MonsterImmunePoison));
    check("no fire immunity", !monsterIsImmune(g_record, MonsterImmuneFire));
    check("no resistances yet", !monsterResistsMagic(g_record) && !monsterResistsPhysical(g_record));
    monsterSetU16(g_record, MonsterFieldResistances, 0x8000);
    check("0x8000 is a physical resistance", monsterResistsPhysical(g_record) && !monsterResistsMagic(g_record));
    monsterSetU16(g_record, MonsterFieldResistances, 0x2000);
    check("0x2000 is a magic resistance", monsterResistsMagic(g_record) && !monsterResistsPhysical(g_record));

    check("loot pointers follow the field order",
          monsterLoot(g_record, MonsterLootGold) == g_record + 0x7E &&
              monsterLoot(g_record, MonsterLootNuore) == g_record + 0x82 &&
              monsterLoot(g_record, MonsterLootOre) == g_record + 0x86 &&
              monsterLoot(g_record, MonsterLootExperience) == g_record + 0x8A);
}

static void setLine(uint8_t *record, unsigned offset, const char *text) {
    memset(record + offset, ' ', 12);
    memcpy(record + offset, text, strlen(text));
    record[offset + 12] = 0;
}

static void testNames(void) {
    char name[MonsterNameBufferSize];
    char line[MonsterNameLineSize];
    memset(g_record, 0, sizeof(g_record));

    setLine(g_record, MonsterFieldName1, "CARNIVOROUS");
    setLine(g_record, MonsterFieldName2, "SPIDER");
    monsterGetName(g_record, name);
    check("two lines join with a space", strcmp(name, "CARNIVOROUS SPIDER") == 0);

    setLine(g_record, MonsterFieldName1, "ALLIGATOR");
    setLine(g_record, MonsterFieldName2, "");
    monsterGetName(g_record, name);
    check("a one-line name keeps the separator, like BuildMonsterDisplayName", strcmp(name, "ALLIGATOR ") == 0);

    monsterGetNameLine(g_record, 0, line);
    check("line 0 is trimmed", strcmp(line, "ALLIGATOR") == 0);
    monsterGetNameLine(g_record, 1, line);
    check("an empty line 1 is empty", line[0] == '\0');
    monsterGetNameLine(g_record, 2, line);
    check("line 2 does not exist", line[0] == '\0');

    memset(g_record + MonsterFieldName1, 'X', 12);
    memset(g_record + MonsterFieldName2, 'Y', 12);
    g_record[MonsterFieldName1 + 12] = g_record[MonsterFieldName2 + 12] = 0;
    monsterGetName(g_record, name);
    checkU32("the longest name is 25 characters", (uint32_t)strlen(name), 25);
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return monsterCatalogReadWorldDatFile(&g_catalog, game, path);
}

static bool blockNameIs(unsigned index, const char *expected) {
    uint8_t record[MonsterRecordSize];
    char name[MonsterNameBufferSize];
    memset(record, 0, sizeof(record));
    memcpy(record + MonsterBlockOffset, monsterCatalogBlock(&g_catalog, index), MonsterBlockSize);
    monsterGetName(record, name);
    return strcmp(name, expected) == 0;
}

static void loadBlock(uint8_t *record, unsigned index) {
    memset(record, 0, MonsterRecordSize);
    memcpy(record + MonsterBlockOffset, monsterCatalogBlock(&g_catalog, index), MonsterBlockSize);
}

static bool validBcd(const uint8_t *value) {
    for (int i = 0; i < 4; i++) {
        if ((value[i] >> 4) > 9 || (value[i] & 15) > 9) {
            return false;
        }
    }
    return true;
}

static uint32_t bcdHex(const uint8_t *value) {
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static void checkInvariants(const char *game, GameKind kind) {
    char label[96];
    bool lootOk = true;
    bool namesOk = true;
    bool lookupOk = true;
    bool healthOk = true;
    unsigned named = 0;

    for (unsigned i = 1; i < g_catalog.blockCount; i++) {
        uint8_t record[MonsterRecordSize];
        char name[MonsterNameBufferSize];
        loadBlock(record, i);
        monsterGetName(record, name);
        if (name[0] == ' ' || name[0] == '\0') {
            continue;
        }
        named++;
        for (const char *c = name; *c; c++) {
            if (*c < ' ' || *c > '~') {
                namesOk = false;
            }
        }
        for (int k = 0; k < 4; k++) {
            if (!validBcd(monsterLoot(record, (MonsterLoot)k))) {
                lootOk = false;
            }
        }
        if (monsterGetU16(record, MonsterFieldMaxHealth) == 0 && strcmp(name, "NOT USED ") != 0) {
            healthOk = false;
        }
    }
    for (unsigned type = 0; type < g_catalog.lookupCount; type++) {
        unsigned raw = g_catalog.lookup[type * 2] | (g_catalog.lookup[type * 2 + 1] << 8);
        if (raw >= g_catalog.blockCount) {
            lookupOk = false;
        }
    }

    snprintf(label, sizeof(label), "%s: every named monster has printable text", game);
    check(label, namesOk);
    snprintf(label, sizeof(label), "%s: every loot value is valid packed BCD", game);
    check(label, lootOk);
    snprintf(label, sizeof(label), "%s: every real monster has hit points", game);
    check(label, healthOk);
    snprintf(label, sizeof(label), "%s: every lookup entry is a block index", game);
    check(label, lookupOk);
    snprintf(label, sizeof(label), "%s: every block after 0 is named except NOT USED filler", game);
    check(label, named + 2 >= g_catalog.blockCount);

    /* Each death-flag type must be a monster the lookup knows. */
    bool flagTypesOk = true;
    unsigned flagTypes = 0;
    for (unsigned type = 0; type < g_catalog.lookupCount; type++) {
        int16_t a, b;
        if (monsterDeathFlags(kind, type, &a, &b)) {
            flagTypes++;
            if (monsterCatalogBlockIndex(&g_catalog, type) == 0 || a == 0) {
                flagTypesOk = false;
            }
        }
    }
    snprintf(label, sizeof(label), "%s: every death-flag type is a real monster with a flag", game);
    check(label, flagTypesOk);
    checkU32("...and the table has the expected number of entries", flagTypes, kind == GameYendor2 ? 17 : 23);
}

static void testRealYendor2(void) {
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game")) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    uint8_t rec[MonsterRecordSize];

    check("block 0 is empty", blockNameIs(0, " "));
    check("block 1 is ALLIGATOR", blockNameIs(1, "ALLIGATOR "));
    check("block 18 is GNOME MINER", blockNameIs(18, "GNOME MINER "));
    check("block 61 is RED DRAGON", blockNameIs(61, "RED DRAGON "));

    loadBlock(rec, 1);
    checkU32("ALLIGATOR sprite base", monsterGetU16(rec, MonsterFieldSpriteBase), 10);
    checkU32("ALLIGATOR health", monsterGetU16(rec, MonsterFieldMaxHealth), 140);
    checkU32("ALLIGATOR save difficulty", monsterGetU16(rec, MonsterFieldSaveDifficulty), 13);
    checkU32("ALLIGATOR accuracy", monsterGetU16(rec, MonsterFieldAccuracy), 108);
    checkU32("ALLIGATOR dexterity", monsterGetU16(rec, MonsterFieldDexterity), 105);
    checkU32("ALLIGATOR absorption", monsterGetU16(rec, MonsterFieldAbsorption), 40);
    checkU32("ALLIGATOR damage", monsterGetU16(rec, MonsterFieldDamage), 70);
    checkU32("ALLIGATOR hit sound", monsterGetU16(rec, MonsterFieldHitSound), 43);
    checkU32("ALLIGATOR idle sound", monsterGetU16(rec, MonsterFieldIdleSound), 43);
    checkU32("ALLIGATOR gold", bcdHex(monsterLoot(rec, MonsterLootGold)), 0x00000495);
    checkU32("ALLIGATOR nuore", bcdHex(monsterLoot(rec, MonsterLootNuore)), 0x00000010);
    checkU32("ALLIGATOR magic ore", bcdHex(monsterLoot(rec, MonsterLootOre)), 0x00000005);
    checkU32("ALLIGATOR experience", bcdHex(monsterLoot(rec, MonsterLootExperience)), 0x00001340);
    checkU32("ALLIGATOR flags", monsterGetU16(rec, MonsterFieldFlags), 0x21);
    check("ALLIGATOR is cold-immune and physically resistant",
          monsterIsImmune(rec, MonsterImmuneCold) && !monsterIsImmune(rec, MonsterImmunePoison) &&
              monsterResistsPhysical(rec) && !monsterResistsMagic(rec));

    loadBlock(rec, 27);
    check("block 27 is MIST DRAGON", blockNameIs(27, "MIST DRAGON "));
    checkU32("MIST DRAGON special attack", monsterGetU16(rec, MonsterFieldSpecialAttack), 40);
    checkU32("MIST DRAGON ranged accuracy", monsterGetU16(rec, MonsterFieldRangedAccuracy), 185);
    checkU32("MIST DRAGON ranged damage", monsterGetU16(rec, MonsterFieldRangedDamage), 170);
    check("MIST DRAGON is immune to paralysis and freezing and resists magic",
          monsterIsImmune(rec, MonsterImmuneParalysis) && monsterIsImmune(rec, MonsterImmuneFreezing) &&
              monsterIsImmune(rec, MonsterImmuneMagicResist) && monsterResistsMagic(rec));
    checkU32("MIST DRAGON drops 10000 gold", bcdHex(monsterLoot(rec, MonsterLootGold)), 0x00010000);

    loadBlock(rec, 61);
    checkU32("RED DRAGON health", monsterGetU16(rec, MonsterFieldMaxHealth), 10000);
    checkU32("RED DRAGON drops 1,000,000 gold", bcdHex(monsterLoot(rec, MonsterLootGold)), 0x01000000);
    check("RED DRAGON is immune to poison through power (0xFC00)",
          monsterIsImmune(rec, MonsterImmunePoison) && monsterIsImmune(rec, MonsterImmuneCursing));

    checkU32("type 1 is WORKER ANT's block", monsterCatalogBlockIndex(&g_catalog, 1), 43);
    check("...WORKER ANT", blockNameIs(43, "WORKER ANT "));
    checkU32("type 10", monsterCatalogBlockIndex(&g_catalog, 10), 37);
    checkU32("type 207", monsterCatalogBlockIndex(&g_catalog, 207), 49);
    checkU32("type 1959", monsterCatalogBlockIndex(&g_catalog, 1959), 5);
    checkU32("type 2143 is the last real type", monsterCatalogBlockIndex(&g_catalog, 2143), 61);
    checkU32("type 2144 is unused", monsterCatalogBlockIndex(&g_catalog, 2144), 0);

    check("spawning an unknown type fails", !monsterRecordSpawn(rec, &g_catalog, 2144));
    memset(rec, 0xEE, sizeof(rec));
    check("spawning type 15 succeeds", monsterRecordSpawn(rec, &g_catalog, 15));
    checkU32("spawned type id", monsterGetU16(rec, MonsterFieldType), 15);
    checkU32("spawn sets health to maximum", monsterGetU16(rec, MonsterFieldHealth),
             monsterGetU16(rec, MonsterFieldMaxHealth));
    checkU32("type 15's death sets global flag 14", monsterGetU16(rec, MonsterFieldFlagOnDeath), 14);
    checkU32("no second flag", monsterGetU16(rec, MonsterFieldFlagOnDeath2), 0);
    checkU32("spawn clears the rest of the runtime state", monsterGetU16(rec, MonsterFieldWorldX), 0);
    {
        char spawned[MonsterNameBufferSize];
        uint8_t expected[MonsterRecordSize];
        char expectedName[MonsterNameBufferSize];
        loadBlock(expected, 37);
        monsterGetName(rec, spawned);
        monsterGetName(expected, expectedName);
        check("spawned record carries block 37's name and stats",
              strcmp(spawned, expectedName) == 0 &&
                  memcmp(rec + MonsterBlockOffset, expected + MonsterBlockOffset, MonsterBlockSize) == 0);
    }
    check("type 1 has no death flag", monsterRecordSpawn(rec, &g_catalog, 1) && monsterGetU16(rec, MonsterFieldFlagOnDeath) == 0);

    checkInvariants("yendor2", GameYendor2);
}

static void testRealYendor3(void) {
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game")) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    uint8_t rec[MonsterRecordSize];

    check("block 1 is WASP", blockNameIs(1, "WASP "));
    check("block 72 is PURPLE DRAGON", blockNameIs(72, "PURPLE DRAGON"));

    loadBlock(rec, 1);
    checkU32("WASP health", monsterGetU16(rec, MonsterFieldMaxHealth), 9);
    checkU32("WASP save difficulty", monsterGetU16(rec, MonsterFieldSaveDifficulty), 1);
    checkU32("WASP accuracy", monsterGetU16(rec, MonsterFieldAccuracy), 65);
    checkU32("WASP dexterity", monsterGetU16(rec, MonsterFieldDexterity), 58);
    checkU32("WASP absorption", monsterGetU16(rec, MonsterFieldAbsorption), 7);
    checkU32("WASP damage", monsterGetU16(rec, MonsterFieldDamage), 5);
    checkU32("WASP sound", monsterGetU16(rec, MonsterFieldHitSound), 36);
    checkU32("WASP gold", bcdHex(monsterLoot(rec, MonsterLootGold)), 0x30);
    checkU32("WASP nuore", bcdHex(monsterLoot(rec, MonsterLootNuore)), 0x03);
    checkU32("WASP experience", bcdHex(monsterLoot(rec, MonsterLootExperience)), 0x15);
    checkU32("WASP flags", monsterGetU16(rec, MonsterFieldFlags), 0xC011);

    loadBlock(rec, 72);
    checkU32("PURPLE DRAGON health", monsterGetU16(rec, MonsterFieldMaxHealth), 305);
    checkU32("PURPLE DRAGON special attack", monsterGetU16(rec, MonsterFieldSpecialAttack), 36);
    checkU32("PURPLE DRAGON gold", bcdHex(monsterLoot(rec, MonsterLootGold)), 0x3700);
    check("PURPLE DRAGON is immune to poison through power", monsterIsImmune(rec, MonsterImmunePoison));

    checkU32("type 1 is block 2 (CENTIPEDE)", monsterCatalogBlockIndex(&g_catalog, 1), 2);
    check("...CENTIPEDE", blockNameIs(2, "CENTIPEDE "));
    checkU32("type 6 is block 1 (WASP)", monsterCatalogBlockIndex(&g_catalog, 6), 1);
    checkU32("type 1862 is block 63", monsterCatalogBlockIndex(&g_catalog, 1862), 63);
    checkU32("types past 1862 are outside the lookup", monsterCatalogBlockIndex(&g_catalog, 1863), 0);
    checkU32("type 1866 is not read from the constants that follow", monsterCatalogBlockIndex(&g_catalog, 1866), 0);

    int16_t a, b;
    check("type 1 sets flag 1 on death in chapter 3", monsterDeathFlags(GameYendor3, 1, &a, &b) && a == 1 && b == 0);
    check("type 1862 sets flag 224", monsterDeathFlags(GameYendor3, 1862, &a, &b) && a == 224);
    check("chapter 2's type 1 has no death flag", !monsterDeathFlags(GameYendor2, 1, &a, &b));
    check("type 2 has no death flag", !monsterDeathFlags(GameYendor3, 2, &a, &b));

    checkInvariants("yendor3", GameYendor3);
}

static void testTickTimer(void) {
    /*
     * 0xFC10 (the gate mask) and 0x3010 (the double-decrement mask)
     * overlap (both include bit 0x10) and 0x3ED (bits kept across a
     * reset) is disjoint from both. To test each condition in
     * isolation: 0x0400 gates the tick without triggering a second
     * decrement (in 0xFC10, not in 0x3010, not in 0x3ED -- also
     * conveniently exercises "cleared on reset"); 0x0001 is a state bit
     * that's in neither mask, to confirm it survives a reset (it's in
     * 0x3ED); 0x3010 itself exercises the double-decrement path.
     */
    uint8_t record[MonsterRecordSize];

    memset(record, 0, sizeof(record));
    check("no gate bits set is a no-op", monsterTickTimer(record) == MonsterTickIdle);

    /* Gate bit set without the double-decrement bits: single decrement, stays positive, countdown not yet 0. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, 0x0400 | 0x0001);
    monsterSetU16(record, MonsterFieldHealth, 100);
    monsterSetU16(record, MonsterFieldTickAmount, 10);
    monsterSetU16(record, MonsterFieldTickCountdown, 5);
    check("single decrement, still positive, countdown not expired -> idle", monsterTickTimer(record) == MonsterTickIdle);
    checkU32("health decremented once", monsterGetU16(record, MonsterFieldHealth), 90);
    checkU32("countdown decremented", monsterGetU16(record, MonsterFieldTickCountdown), 4);
    checkU32("state bits untouched (no reset yet)", monsterGetU16(record, MonsterFieldState), 0x0400 | 0x0001);

    /* Countdown reaching 0 triggers the reset, even though health is still positive. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, 0x0400 | 0x0001); /* 0x400 is outside mask 0x3ED, 0x1 is inside it */
    monsterSetU16(record, MonsterFieldHealth, 100);
    monsterSetU16(record, MonsterFieldTickAmount, 10);
    monsterSetU16(record, MonsterFieldTickCountdown, 1);
    monsterSetU16(record, MonsterFieldSpriteBase, 42);
    monsterSetU16(record, MonsterFieldAnim, 47);
    check("countdown reaching 0 still reports idle", monsterTickTimer(record) == MonsterTickIdle);
    checkU32("...clears 0x400 (outside the 0x3ED mask) but keeps 0x1 (inside it)", monsterGetU16(record, MonsterFieldState),
             0x0001);
    checkU32("...zeroes the tick target", monsterGetU16(record, MonsterFieldTickTarget), 0);
    checkU32("...zeroes the tick amount", monsterGetU16(record, MonsterFieldTickAmount), 0);
    checkU32("...zeroes the countdown", monsterGetU16(record, MonsterFieldTickCountdown), 0);
    checkU32("...resets the animation to the sprite base", monsterGetU16(record, MonsterFieldAnim), 42);

    /* A single decrement that reaches zero or below expires the monster immediately. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, 0x0400);
    monsterSetU16(record, MonsterFieldHealth, 5);
    monsterSetU16(record, MonsterFieldTickAmount, 10);
    check("a decrement past zero expires the monster", monsterTickTimer(record) == MonsterTickExpired);
    checkU32("health is clamped to 0 on expiry", monsterGetU16(record, MonsterFieldHealth), 0);

    /* State bits 0x3010 set: a second decrement is applied; if it survives, result is Ongoing. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, 0x3010);
    monsterSetU16(record, MonsterFieldHealth, 100);
    monsterSetU16(record, MonsterFieldTickAmount, 10);
    monsterSetU16(record, MonsterFieldTickCountdown, 5);
    check("double decrement, still positive -> ongoing", monsterTickTimer(record) == MonsterTickOngoing);
    checkU32("health decremented twice", monsterGetU16(record, MonsterFieldHealth), 80);

    /* Double decrement that crosses zero on the second subtraction still expires. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, 0x3010);
    monsterSetU16(record, MonsterFieldHealth, 15);
    monsterSetU16(record, MonsterFieldTickAmount, 10);
    check("double decrement crossing zero on the second subtraction expires",
          monsterTickTimer(record) == MonsterTickExpired);
    checkU32("health is clamped to 0", monsterGetU16(record, MonsterFieldHealth), 0);
}

static void testActivateByDistance(void) {
    uint8_t record[MonsterRecordSize];

    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldState, MonsterStateAware);
    check("an already-aware monster never re-activates, regardless of depth",
          !monsterTryActivateByDistance(record, 0xFFFF));

    memset(record, 0, sizeof(record));
    check("below the baseline threshold (<=0x21), never activates even with no tier bits",
          !monsterTryActivateByDistance(record, 0x21));
    checkU32("...state untouched", monsterGetU16(record, MonsterFieldState), 0);

    memset(record, 0, sizeof(record));
    check("just above the baseline, no tier bits set: activates", monsterTryActivateByDistance(record, 0x22));
    check("...MonsterStateAware is now set", (monsterGetU16(record, MonsterFieldState) & MonsterStateAware) != 0);

    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldAwareness, MonsterAwarenessNever);
    check("MonsterAwarenessNever blocks activation even far past the baseline",
          !monsterTryActivateByDistance(record, 0xFFFF));

    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldAwareness, MonsterAwarenessFar);
    check("Far tier: not yet at its own threshold (0x2C)", !monsterTryActivateByDistance(record, 0x2C));
    check("Far tier: activates just past 0x2C", monsterTryActivateByDistance(record, 0x2D));

    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldAwareness, MonsterAwarenessMiddle);
    check("Middle tier: not yet at its own threshold (0x29)", !monsterTryActivateByDistance(record, 0x29));
    check("Middle tier: activates just past 0x29", monsterTryActivateByDistance(record, 0x2A));

    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldAwareness, MonsterAwarenessNear);
    check("Near tier: not yet at its own threshold (0x26)", !monsterTryActivateByDistance(record, 0x26));
    check("Near tier: activates just past 0x26", monsterTryActivateByDistance(record, 0x27));

    /* Far takes priority over Middle/Near when multiple tier bits are (unusually) set together. */
    memset(record, 0, sizeof(record));
    monsterSetU16(record, MonsterFieldAwareness, (uint16_t)(MonsterAwarenessFar | MonsterAwarenessNear));
    check("Far's stricter threshold wins when combined with Near",
          !monsterTryActivateByDistance(record, 0x27)); /* would satisfy Near's threshold but not Far's */
}

int main(void) {
    testLayouts();
    testParse();
    testRecord();
    testNames();
    testTickTimer();
    testActivateByDistance();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_party test_party.c ../party.c ../savegame.c ../savegame_stdio.c ../bcd4.c && ./test_party
 *
 * Real-character checks read yendor2/game/CURGAME (gitignored; skipped if
 * absent). Set YENDOR2_GAME_DIR to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd4.h"
#include "party.h"
#include "savegame.h"
#include "savegame_stdio.h"

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

static uint8_t g_record[PartyRecordSize];

static void testLayoutRelations(void) {
    checkU32("party record size matches the savegame module", PartyRecordSize, SavePartyRecordSize);
    checkU32("item-instance size equals an inventory group", InventoryGroupSize, SaveItemInstanceSize);
    checkU32("main inventory group ends where equipment starts",
             PartyFieldInventory + InventoryGroupSize, PartyFieldEquipment);
    checkU32("stat array ends before the base-rating block",
             PartyFieldStats + PartyStatCount * 2 - 1, 0x71);
    checkU32("max stat array is the current array + 0x40", PartyFieldStatsMax - PartyFieldStats,
             PartyMaxStatOffset);
    checkU32("bag 1 contents are at 0x180",
             (uint32_t)(partyInventoryGroup(g_record, PartyGroupBag1) - g_record), 0x180);
    checkU32("bag 2 contents are at 0x1A6",
             (uint32_t)(partyInventoryGroup(g_record, PartyGroupBag2) - g_record), 0x1A6);
    checkU32("bag 3 contents are at 0x1CC",
             (uint32_t)(partyInventoryGroup(g_record, PartyGroupBag3) - g_record), 0x1CC);
    check("the last bag fits in the record",
          (partyBagMarker(g_record, 2) - g_record) + PartyBagMarkerSize <= PartyRecordSize);
    check("bag 3 marker is out of range", partyBagMarker(g_record, 3) == NULL);
    checkU32("HP and MP stat offsets", PartyFieldStats + PartyStatHitPoints * 2, 0x52);
    checkU32("MP stat offset", PartyFieldStats + PartyStatMagicPoints * 2, 0x54);
    checkU32("bartering is +0x68", PartyFieldStats + PartyStatBartering * 2, 0x68);
    checkU32("chemistry max is +0xB0", PartyFieldStatsMax + PartyStatChemistry * 2, 0xB0);
}

static void testStats(void) {
    memset(g_record, 0, sizeof(g_record));
    partySetStat(g_record, PartyStatStrength, 0x1234);
    partySetStatMax(g_record, PartyStatStrength, 0x5678);
    check("current strength is at +0x3C and little-endian",
          g_record[0x3C] == 0x34 && g_record[0x3D] == 0x12);
    check("max strength is at +0x7C", g_record[0x7C] == 0x78 && g_record[0x7D] == 0x56);
    checkU32("stat reads back", partyGetStat(g_record, PartyStatStrength), 0x1234);
    checkU32("stat max reads back", partyGetStatMax(g_record, PartyStatStrength), 0x5678);
    checkU32("out-of-range stat reads as 0", partyGetStat(g_record, PartyStatCount), 0);
    partySetStat(g_record, PartyStatCount, 1);
    check("out-of-range set changes nothing", g_record[0x3C + PartyStatCount * 2] == 0);

    check("STRENGTH", strcmp(partyStatName(PartyStatStrength, GameYendor2), "STRENGTH") == 0);
    check("BARTERING", strcmp(partyStatName(PartyStatBartering, GameYendor2), "BARTERING") == 0);
    check("CHEMISTRY", strcmp(partyStatName(PartyStatChemistry, GameYendor2), "CHEMISTRY") == 0);
    check("yendor2 names hit points HIT POINTS", strcmp(partyStatName(PartyStatHitPoints, GameYendor2), "HIT POINTS") == 0);
    check("yendor3 names hit points HEALTH", strcmp(partyStatName(PartyStatHitPoints, GameYendor3), "HEALTH") == 0);
    check("yendor3 has no chemistry skill name", partyStatName(PartyStatChemistry, GameYendor3) == NULL);
    check("yendor3 keeps the other names", strcmp(partyStatName(PartyStatLinguistics, GameYendor3), "LINGUISTICS") == 0);
    check("unnamed stat has no name", partyStatName(PartyStatEquipRating1, GameYendor2) == NULL);
    check("carry capacity has no table name", partyStatName(PartyStatCarryCapacity, GameYendor2) == NULL);

    partySetU16(g_record, PartyFieldProtections + 2 * PartyProtectionHexing, 7);
    checkU32("protection order: hexing is the 8th value", partyGetProtection(g_record, PartyProtectionHexing), 7);
    checkU32("hexing offset is +0x2E", PartyFieldProtections + 2 * PartyProtectionHexing, 0x2E);
}

static void testClasses(void) {
    check("1 is FIGHTER", strcmp(partyClassName(1), "FIGHTER") == 0);
    check("9 is MARKSMAN", strcmp(partyClassName(9), "MARKSMAN") == 0);
    check("11 is WARRIOR", strcmp(partyClassName(11), "WARRIOR") == 0);
    check("19 is RANGER", strcmp(partyClassName(19), "RANGER") == 0);
    check("21 is CHAMPION", strcmp(partyClassName(21), "CHAMPION") == 0);
    check("29 is KNIGHT", strcmp(partyClassName(29), "KNIGHT") == 0);
    check("0, 10, 20 and 30+ are invalid",
          !partyClassIsValid(0) && !partyClassIsValid(10) && !partyClassIsValid(20) &&
              !partyClassIsValid(30) && partyClassName(10) == NULL);
    checkU32("class 17 is tier 1", partyClassTier(17), 1);
    checkU32("class 17 has base 7", partyClassBase(17), 7);

    bool allValid = true;
    for (unsigned id = 1; id <= 29; id++) {
        if (id % 10 != 0 && partyClassName(id) == NULL) {
            allValid = false;
        }
    }
    check("all 27 valid class ids have names", allValid);
    checkU32("class 4 secondary bit", partyClassSecondaryBit(4), 0x20);
    checkU32("class 9 secondary bit", partyClassSecondaryBit(9), 0x01);
    checkU32("class 3 has no secondary bit", partyClassSecondaryBit(3), 0);
}

static void testFlagBanks(void) {
    uint8_t bank[32];
    bool exact = true;
    memset(bank, 0, sizeof(bank));

    /* Every flag sets exactly one bit, and it's the one the original computes. */
    for (unsigned index = 1; index <= 256; index++) {
        memset(bank, 0, sizeof(bank));
        flagBankSet(bank, 16, index);
        unsigned word = (index - 1) / 16;
        unsigned bit = 15 - (index - 1) % 16;
        for (unsigned w = 0; w < 16; w++) {
            uint16_t expected = (w == word) ? (uint16_t)(1u << bit) : 0;
            if (partyGetU16(bank, w * 2) != expected) {
                exact = false;
            }
        }
        if (!flagBankTest(bank, 16, index)) {
            exact = false;
        }
    }
    check("256 flags are 1-based, MSB-first, 16 per word", exact);

    memset(bank, 0, sizeof(bank));
    flagBankSet(bank, 16, 1);
    flagBankSet(bank, 16, 16);
    flagBankSet(bank, 16, 17);
    check("flag 1 is 0x8000 of word 0", partyGetU16(bank, 0) == 0x8001);
    check("flag 17 is 0x8000 of word 1", partyGetU16(bank, 2) == 0x8000);
    flagBankClear(bank, 16, 1);
    check("clear removes just that flag", partyGetU16(bank, 0) == 0x0001);

    memset(bank, 0, sizeof(bank));
    flagBankSet(bank, 16, 0);
    flagBankSet(bank, 16, 257);
    bool untouched = true;
    for (size_t i = 0; i < sizeof(bank); i++) {
        if (bank[i] != 0) {
            untouched = false;
        }
    }
    check("index 0 and 257 are ignored", untouched);
    check("out-of-range tests are false", !flagBankTest(bank, 16, 0) && !flagBankTest(bank, 16, 257));

    memset(g_record, 0, sizeof(g_record));
    partySetAbilityFlag(g_record, 1);
    partySetEventFlag(g_record, 96);
    check("ability flags live at +0xCA", g_record[0xCA + 1] == 0x80 && partyTestAbilityFlag(g_record, 1));
    check("event flag 96 is the last bit of +0x10C bank",
          g_record[0x10C + 10] == 0x01 && g_record[0x10C + 11] == 0x00 && partyTestEventFlag(g_record, 96));
    check("event bank has only 96 flags", !partyTestEventFlag(g_record, 97));
}

static void testInventory(void) {
    memset(g_record, 0, sizeof(g_record));

    uint8_t *main = partyInventoryGroup(g_record, PartyGroupMain);
    check("main group is at +0x118", main == g_record + 0x118);
    check("slot 1 is 2 bytes into the group", inventoryGroupSlot(main, 1) == main + 2);
    check("slot 8 is at +0x1E within the group", inventoryGroupSlot(main, 8) == main + 0x1E);
    check("slot 8 ends at the group's end", inventoryGroupSlot(main, 8) + ItemSlotSize == main + InventoryGroupSize);
    check("slot 0 and 9 are invalid", inventoryGroupSlot(main, 0) == NULL && inventoryGroupSlot(main, 9) == NULL);

    itemSlotSet(inventoryGroupSlot(main, 3), 0x21E, 7);
    checkU32("item id round-trips", itemSlotId(inventoryGroupSlot(main, 3)), 0x21E);
    checkU32("item extra round-trips", itemSlotExtra(inventoryGroupSlot(main, 3)), 7);
    check("slot 3 is at +0x122 of the record", g_record[0x122] == 0x1E && g_record[0x123] == 0x02);
    inventoryGroupSetWeight(main, 71);
    checkU32("weight is the group's first word", partyGetU16(g_record, 0x118), 71);

    check("nothing open: main group is active", partyActiveInventoryGroup(g_record) == main);
    partySetU16(partyBagMarker(g_record, 1), 0, 0x150);
    check("open bag 2 becomes active", partyActiveInventoryGroup(g_record) == g_record + 0x1A6);
    partySetU16(partyBagMarker(g_record, 0), 0, 0x151);
    check("bag 1 outranks bag 2", partyActiveInventoryGroup(g_record) == g_record + 0x180);
    partySetU16(partyBagMarker(g_record, 0), 0, 0);
    partySetU16(partyBagMarker(g_record, 1), 0, 0);
    partySetU16(partyBagMarker(g_record, 2), 0, 0x152);
    check("open bag 3 becomes active", partyActiveInventoryGroup(g_record) == g_record + 0x1CC);

    static const unsigned longOffsets[] = {0x13A, 0x13E, 0x142, 0x146, 0x14A, 0x14E};
    bool equipmentOk = true;
    for (unsigned i = 0; i < 6; i++) {
        if (partyEquipmentSlot(g_record, 0x0A + i, GameYendor2) != g_record + longOffsets[i]) {
            equipmentOk = false;
        }
    }
    check("equipment codes 0xA-0xF are 4-byte slots at 0x13A..0x14E", equipmentOk);
    static const unsigned shortOffsets[] = {0x152, 0x154, 0x156, 0x158, 0x15A};
    for (unsigned i = 0; i < 5; i++) {
        if (partyEquipmentSlot(g_record, 0x10 + i, GameYendor2) != g_record + shortOffsets[i]) {
            equipmentOk = false;
        }
    }
    check("equipment codes 0x10-0x14 are 2-byte slots at 0x152..0x15A", equipmentOk);
    check("codes 9 and 0x15 are not equipment",
          partyEquipmentSlot(g_record, 9, GameYendor2) == NULL && partyEquipmentSlot(g_record, 0x15, GameYendor2) == NULL);

    /* Chapter 3's slot lookup has no case for code 0x12: it lands on 0x15A, like 0x14. */
    check("yendor2: code 0x12 is slot 0x156", partyEquipmentSlot(g_record, 0x12, GameYendor2) == g_record + 0x156);
    check("yendor3: code 0x12 aliases slot 0x15A", partyEquipmentSlot(g_record, 0x12, GameYendor3) == g_record + 0x15A);
    check("yendor3: code 0x14 is still 0x15A", partyEquipmentSlot(g_record, 0x14, GameYendor3) == g_record + 0x15A);
    check("yendor3: other codes are unchanged",
          partyEquipmentSlot(g_record, 0x0A, GameYendor3) == g_record + 0x13A &&
              partyEquipmentSlot(g_record, 0x11, GameYendor3) == g_record + 0x154 &&
              partyEquipmentSlot(g_record, 0x13, GameYendor3) == g_record + 0x158);
}

static SaveGame g_save;

static bool loadRealParty(void) {
    char path[512];
    const char *dir = getenv("YENDOR2_GAME_DIR");
    if (!dir) {
        dir = "../../yendor2/game";
    }
    snprintf(path, sizeof(path), "%s/CURGAME", dir);
    return saveGameReadFile(&g_save, path);
}

static void testRealCharacters(void) {
    if (!loadRealParty()) {
        printf("SKIP real-character checks (no CURGAME found)\n");
        g_skipCount++;
        return;
    }

    static const char *const names[] = {"SQUIRE", "DIANA", "YENDOR", "JOSEPHINE"};
    static const unsigned classes[] = {2, 4, 7, 8};
    static const unsigned genders[] = {1, 2, 1, 2};
    static const unsigned weights[] = {71, 71, 69, 48};
    /* Ability/spell flags granted to each class; 1-based flag indices. */
    static const unsigned flagA[] = {0, 1, 2, 1};
    static const unsigned flagB[] = {0, 3, 3, 2};

    for (unsigned i = 0; i < 4; i++) {
        uint8_t *rec = saveGamePartyRecord(&g_save, 5 + i);
        char name[PartyNameBufferSize];
        char label[96];

        partyGetName(rec, name);
        snprintf(label, sizeof(label), "%s: name", names[i]);
        check(label, strcmp(name, names[i]) == 0);
        snprintf(label, sizeof(label), "%s: class %u is %s", names[i], classes[i], partyClassName(classes[i]));
        checkU32(label, partyGetU16(rec, PartyFieldClass), classes[i]);
        snprintf(label, sizeof(label), "%s: gender", names[i]);
        checkU32(label, partyGetU16(rec, PartyFieldGender), genders[i]);
        snprintf(label, sizeof(label), "%s: level 1", names[i]);
        checkU32(label, partyGetU16(rec, PartyFieldLevel), 1);
        snprintf(label, sizeof(label), "%s: experience is 0", names[i]);
        checkU32(label, bcd4Compare(partyExperience(rec), (Bcd4){0, 0, 0, 0}), 0);

        snprintf(label, sizeof(label), "%s: carry capacity is 10 x strength", names[i]);
        checkU32(label, partyGetStat(rec, PartyStatCarryCapacity), 10u * partyGetStat(rec, PartyStatStrength));
        bool currentIsMax = true;
        for (int s = 0; s < PartyStatCount; s++) {
            if (partyGetStat(rec, (PartyStat)s) != partyGetStatMax(rec, (PartyStat)s)) {
                currentIsMax = false;
            }
        }
        snprintf(label, sizeof(label), "%s: every stat starts at its maximum", names[i]);
        check(label, currentIsMax);

        snprintf(label, sizeof(label), "%s: secondary-class bit matches class", names[i]);
        checkU32(label, partyGetU16(rec, PartyFieldStatusFlags) & PartyStatusSecondaryClassMask,
                 partyClassSecondaryBit(classes[i]));

        snprintf(label, sizeof(label), "%s: main weapon slot holds item 0x21E", names[i]);
        checkU32(label, itemSlotId(partyEquipmentSlot(rec, 0x0A, GameYendor2)), 0x21E);
        snprintf(label, sizeof(label), "%s: carried weight", names[i]);
        checkU32(label, inventoryGroupWeight(partyInventoryGroup(rec, PartyGroupMain)), weights[i]);
        snprintf(label, sizeof(label), "%s: no bag is open", names[i]);
        check(label, partyActiveInventoryGroup(rec) == partyInventoryGroup(rec, PartyGroupMain));

        unsigned setCount = 0;
        for (unsigned f = 1; f <= 256; f++) {
            if (partyTestAbilityFlag(rec, f)) {
                setCount++;
            }
        }
        snprintf(label, sizeof(label), "%s: ability flags are exactly {%u,%u}", names[i], flagA[i], flagB[i]);
        check(label, setCount == (flagA[i] ? 2u : 0u) &&
                         (flagA[i] == 0 || (partyTestAbilityFlag(rec, flagA[i]) && partyTestAbilityFlag(rec, flagB[i]))));
    }

    uint8_t *squire = saveGamePartyRecord(&g_save, 5);
    check("SQUIRE has no magic points", partyGetStat(squire, PartyStatMagicPoints) == 0);
    check("SQUIRE's casting skill is 0", partyGetStat(squire, PartyStatCasting) == 0);
    uint8_t *diana = saveGamePartyRecord(&g_save, 6);
    check("DIANA has 12 magic points", partyGetStatMax(diana, PartyStatMagicPoints) == 12);
    uint8_t *yendor = saveGamePartyRecord(&g_save, 7);
    check("YENDOR has item 6 (extra 4) in equipment slot 0xB",
          itemSlotId(partyEquipmentSlot(yendor, 0x0B, GameYendor2)) == 6 && itemSlotExtra(partyEquipmentSlot(yendor, 0x0B, GameYendor2)) == 4);
}

static void setExperience(uint8_t *record, const uint8_t bcd[4]) {
    memcpy(partyExperience(record), bcd, 4);
}

static void testLevelUp(void) {
    memset(g_record, 0, sizeof(g_record));
    partySetU16(g_record, PartyFieldLevel, 1);
    setExperience(g_record, (const uint8_t[4]){0x00, 0x00, 0x00, 0x00});
    check("below the first threshold: no level up", !partyCheckForLevelUp(g_record, GameYendor2));
    checkU32("pending level stays 0", partyGetU16(g_record, PartyFieldPendingLevel), 0);

    partySetU16(g_record, PartyFieldLevel, 1);
    setExperience(g_record, (const uint8_t[4]){0x00, 0x00, 0x06, 0x80}); /* == table[0], the >= boundary */
    check("exactly at the first threshold: one level up", partyCheckForLevelUp(g_record, GameYendor2));
    checkU32("pending level 2", partyGetU16(g_record, PartyFieldPendingLevel), 2);

    partySetU16(g_record, PartyFieldLevel, 1);
    setExperience(g_record, (const uint8_t[4]){0x00, 0x00, 0x18, 0x50}); /* == table[1] exactly */
    check("landing exactly on a later threshold: cascade stops one level short",
          partyCheckForLevelUp(g_record, GameYendor2));
    checkU32("pending level 2, not 3 (needs strictly > table[1] to cascade further)",
             partyGetU16(g_record, PartyFieldPendingLevel), 2);

    partySetU16(g_record, PartyFieldLevel, 1);
    setExperience(g_record, (const uint8_t[4]){0x00, 0x00, 0x20, 0x00}); /* > table[1] (0x1850), < table[2] (0x2600) */
    check("strictly past a later threshold: cascades one more level",
          partyCheckForLevelUp(g_record, GameYendor2));
    checkU32("pending level 3", partyGetU16(g_record, PartyFieldPendingLevel), 3);

    partySetU16(g_record, PartyFieldLevel, 1);
    partySetU16(g_record, PartyFieldStatusFlags, PartyStatusStoned);
    setExperience(g_record, (const uint8_t[4]){0x99, 0x99, 0x99, 0x99});
    check("incapacitated: no level up regardless of experience", !partyCheckForLevelUp(g_record, GameYendor2));
    partySetU16(g_record, PartyFieldStatusFlags, 0);

    partySetU16(g_record, PartyFieldLevel, 89);
    setExperience(g_record, (const uint8_t[4]){0x99, 0x99, 0x99, 0x99});
    check("level 89 with max experience reaches the level-90 cap", partyCheckForLevelUp(g_record, GameYendor2));
    checkU32("pending level 90", partyGetU16(g_record, PartyFieldPendingLevel), 90);

    partySetU16(g_record, PartyFieldLevel, 90);
    check("level 90 (already at cap, out of table range): guarded, no level up",
          !partyCheckForLevelUp(g_record, GameYendor2));

    partySetU16(g_record, PartyFieldLevel, 0);
    check("level 0: guarded, no level up", !partyCheckForLevelUp(g_record, GameYendor2));

    checkU32("yendor2 and yendor3 tables agree on the first threshold",
             partyXpThresholdTable(GameYendor2)[0][3], partyXpThresholdTable(GameYendor3)[0][3]);
    check("yendor2 and yendor3 caps genuinely differ (90,000,000 vs 99,999,999)",
          memcmp(partyXpThresholdTable(GameYendor2)[88], (const uint8_t[4]){0x90, 0x00, 0x00, 0x00}, 4) == 0 &&
              memcmp(partyXpThresholdTable(GameYendor3)[88], (const uint8_t[4]){0x99, 0x99, 0x99, 0x99}, 4) == 0);
}

int main(void) {
    testLayoutRelations();
    testStats();
    testClasses();
    testFlagBanks();
    testInventory();
    testLevelUp();
    testRealCharacters();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

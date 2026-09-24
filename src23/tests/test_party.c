/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_party test_party.c ../party.c ../savegame.c ../savegame_stdio.c ../bcd4.c ../item.c && ./test_party
 *
 * Real-character checks read yendor2/game/CURGAME (gitignored; skipped if
 * absent). Set YENDOR2_GAME_DIR to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd4.h"
#include "item.h"
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

static void setupTrainee(uint8_t *record, unsigned classId, unsigned level, uint16_t str, uint16_t dex,
                          uint16_t sta, uint16_t intel, uint16_t wis, uint16_t cha, uint16_t hpMax,
                          uint16_t mpMax) {
    memset(record, 0, PartyRecordSize);
    partySetU16(record, PartyFieldClass, (uint16_t)classId);
    partySetU16(record, PartyFieldLevel, (uint16_t)level);
    partySetStatMax(record, PartyStatStrength, str);
    partySetStatMax(record, PartyStatDexterity, dex);
    partySetStatMax(record, PartyStatStamina, sta);
    partySetStatMax(record, PartyStatIntelligence, intel);
    partySetStatMax(record, PartyStatWisdom, wis);
    partySetStatMax(record, PartyStatCharisma, cha);
    partySetStatMax(record, PartyStatHitPoints, hpMax);
    partySetStatMax(record, PartyStatMagicPoints, mpMax);
    partySetStatMax(record, PartyStatSurvival, 5);
    /* PartyStatChemistry left at 0 on purpose: an "untrained skill" that must stay 0. */
}

static uint32_t bcdHex(const uint8_t *value) {
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static void testTraining(void) {
    SaveGame save;
    Bcd4 cost;
    bcd4FromU16(cost, 100);

    /* Insufficient gold: refused, nothing touched. */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 50);
    setupTrainee(g_record, 7 /* MAGE */, 1, 10, 0, 50, 40, 20, 5, 100, 30);
    check("insufficient gold is refused",
          partyApplyTraining(g_record, GameYendor2, &save, cost) == PartyTrainOutcomeInsufficientGold);
    checkU32("gold untouched on refusal", bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x50);
    checkU32("level untouched on refusal", partyGetU16(g_record, PartyFieldLevel), 1);

    /* A full apply: MAGE (base 7, default case), sufficient gold. */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 7, 1, 10, 0, 50, 40, 20, 0, 100, 30);
    check("sufficient gold: applied",
          partyApplyTraining(g_record, GameYendor2, &save, cost) == PartyTrainOutcomeApplied);
    checkU32("gold spent", bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x0900);
    checkU32("level incremented", partyGetU16(g_record, PartyFieldLevel), 2);
    checkU32("max HP grows by 30% of max Stamina (50 -> +15 -> 115)", partyGetStatMax(g_record, PartyStatHitPoints),
             115);
    checkU32("current HP is fully healed to the new max", partyGetStat(g_record, PartyStatHitPoints), 115);
    checkU32("MAGE (base 7): max MP grows by 30% of Intelligence (40 -> +12 -> 42)",
             partyGetStatMax(g_record, PartyStatMagicPoints), 42);
    checkU32("current MP synced to the new max", partyGetStat(g_record, PartyStatMagicPoints), 42);
    checkU32("Strength (nonzero) grows by a flat +2", partyGetStatMax(g_record, PartyStatStrength), 12);
    checkU32("Dexterity (started at 0) stays untrained", partyGetStatMax(g_record, PartyStatDexterity), 0);
    checkU32("Stamina also grows by +2, AFTER being read for the HP calc above",
             partyGetStatMax(g_record, PartyStatStamina), 52);
    checkU32("Survival (nonzero) grows by +2", partyGetStatMax(g_record, PartyStatSurvival), 7);
    checkU32("Chemistry (started at 0) stays untrained", partyGetStatMax(g_record, PartyStatChemistry), 0);

    /* partySyncStagedStats: current values (which all started at 0 in setupTrainee) get pulled up to the new max. */
    checkU32("SyncPartyRecordStagedStats: current Strength synced to its new max",
             partyGetStat(g_record, PartyStatStrength), 12);
    checkU32("current Stamina synced to its new max", partyGetStat(g_record, PartyStatStamina), 52);
    checkU32("current Survival synced to its new max", partyGetStat(g_record, PartyStatSurvival), 7);
    checkU32("current Dexterity stays 0 (its max never left 0)", partyGetStat(g_record, PartyStatDexterity), 0);

    /* partyRefreshCarryCapacityAndAttributeBonuses: recomputed from the post-sync current Strength (12). */
    checkU32("carry capacity (current) is 10x the post-training current Strength",
             partyGetStat(g_record, PartyStatCarryCapacity), 120);
    checkU32("carry capacity (max) is 10x max Strength", partyGetStatMax(g_record, PartyStatCarryCapacity), 120);
    checkU32("Strength bonus is 0 (12 is nowhere near the 72 threshold)",
             partyGetU16(g_record, PartyFieldStrengthBonus), 0);

    /* Physical classes (base 1-3): no MP growth attempted at all, even with nonzero MP max. */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 1 /* FIGHTER */, 1, 10, 10, 10, 10, 10, 10, 100, 30);
    check("FIGHTER: applied", partyApplyTraining(g_record, GameYendor2, &save, cost) == PartyTrainOutcomeApplied);
    checkU32("FIGHTER (base 1): max MP is left completely untouched",
             partyGetStatMax(g_record, PartyStatMagicPoints), 30);
    checkU32("FIGHTER: current MP is untouched too", partyGetStat(g_record, PartyStatMagicPoints), 0);

    /* DRUID (base 8): 30% of (75% Intelligence + 25% Wisdom). */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 8, 1, 10, 10, 10, 100, 40, 10, 100, 500);
    partyApplyTraining(g_record, GameYendor2, &save, cost);
    /* 75%*100=75, 25%*40=10, sum=85, 30%*85=25.5 -> round(85*30/100)=26 (rounds .5 up per (x+50)/100). */
    checkU32("DRUID (base 8): max MP grows by 30% of (75% Int + 25% Wis)",
             partyGetStatMax(g_record, PartyStatMagicPoints), 526);

    /* PALADIN (base 6): 30% of (50% Wisdom). */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 6, 1, 10, 10, 10, 10, 100, 10, 100, 500);
    partyApplyTraining(g_record, GameYendor2, &save, cost);
    /* 50%*100=50, 30%*50=15. */
    checkU32("PALADIN (base 6): max MP grows by 30% of (50% Wisdom)",
             partyGetStatMax(g_record, PartyStatMagicPoints), 515);

    /* Level cap at 90. */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 7, 90, 10, 10, 10, 10, 10, 10, 100, 30);
    partyApplyTraining(g_record, GameYendor2, &save, cost);
    checkU32("level stays capped at 90", partyGetU16(g_record, PartyFieldLevel), 90);

    /* Max HP/MP cap at 9999; attribute/skill cap at 999. */
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 7, 1, 998, 10, 1000, 10, 10, 10, 9990, 30);
    partyApplyTraining(g_record, GameYendor2, &save, cost);
    checkU32("max HP is capped at 9999 (9990 + 300 would overflow)", partyGetStatMax(g_record, PartyStatHitPoints),
             9999);
    checkU32("Strength is capped at 999 (998 + 2 would overflow)", partyGetStatMax(g_record, PartyStatStrength),
             999);

    /* Class promotion at the two Chapter 2 thresholds; Chapter 3's are always-zero (disabled). */
    checkU32("yendor2 promotion thresholds", partyClassPromotionThresholds(GameYendor2)->tier1At, 10);
    checkU32("yendor2 second promotion threshold", partyClassPromotionThresholds(GameYendor2)->tier2At, 30);
    checkU32("yendor3 promotion thresholds are always-zero (disabled)",
             partyClassPromotionThresholds(GameYendor3)->tier1At, 0);

    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 1 /* FIGHTER */, 9, 10, 10, 10, 10, 10, 10, 100, 30);
    partyApplyTraining(g_record, GameYendor2, &save, cost); /* level 9 -> 10 */
    checkU32("yendor2: reaching level 10 promotes FIGHTER (1) to WARRIOR (11)",
             partyGetU16(g_record, PartyFieldClass), 11);
    partyApplyTraining(g_record, GameYendor2, &save, cost); /* level 10 -> 11, no threshold crossed */
    checkU32("no further promotion at level 11", partyGetU16(g_record, PartyFieldClass), 11);

    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 11 /* WARRIOR: tier-1 FIGHTER */, 29, 10, 10, 10, 10, 10, 10, 100, 30);
    partyApplyTraining(g_record, GameYendor2, &save, cost); /* level 29 -> 30 */
    checkU32("yendor2: reaching level 30 promotes a tier-1 class to tier 2 (WARRIOR 11 -> CHAMPION 21)",
             partyGetU16(g_record, PartyFieldClass), 21);

    saveGameInit(&save, GameYendor3);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 1, 9, 10, 10, 10, 10, 10, 10, 100, 30);
    partyApplyTraining(g_record, GameYendor3, &save, cost); /* level 9 -> 10 */
    checkU32("yendor3: reaching level 10 does NOT promote (thresholds always 0)",
             partyGetU16(g_record, PartyFieldClass), 1);

    /* Strength/Dexterity excess-over-72 bonus, exercised directly via the standalone refresh function. */
    memset(g_record, 0, PartyRecordSize);
    partySetStat(g_record, PartyStatStrength, 82); /* current: 20% of (82-72)=10 -> 2 */
    partySetStatMax(g_record, PartyStatStrength, 92); /* max: 20% of (92-72)=20 -> 4 */
    partySetStat(g_record, PartyStatDexterity, 72); /* exactly at the threshold -> 0, not negative */
    partyRefreshCarryCapacityAndAttributeBonuses(g_record);
    checkU32("current Strength bonus: 20% of (82-72)", partyGetU16(g_record, PartyFieldStrengthBonus), 2);
    checkU32("max Strength bonus: 20% of (92-72)", partyGetU16(g_record, PartyFieldStrengthBonusMax), 4);
    checkU32("Dexterity exactly at 72 gets no bonus", partyGetU16(g_record, PartyFieldDexterityBonus), 0);
    checkU32("carry capacity (current) is 10x current Strength (82)", partyGetStat(g_record, PartyStatCarryCapacity),
             820);
}

static void setU16At(uint8_t *base, size_t offset, uint16_t value) {
    base[offset] = (uint8_t)value;
    base[offset + 1] = (uint8_t)(value >> 8);
}

static void setWord(uint8_t *table, size_t offset, unsigned word, uint16_t value) {
    setU16At(table, offset + word * 2, value);
}

static void testEquipmentBonuses(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.itemCount = 4;
    catalog.weaponCount = 2;
    catalog.wearableCount = 2;

    /* Item 1: main weapon. Weapon target entry 0: bonus (word 0) = 5. */
    uint8_t *item1 = catalog.items + 0 * ItemRecordSize;
    setU16At(item1, ItemFieldFlags, ItemFlagEquipCode0A);
    setU16At(item1, ItemFieldTargetOffset, 0);
    setWord(catalog.weapons, 0, ItemTargetAbsorption, 5);

    /* Item 2: off-hand (code 0xC), also weapon-kind. Weapon target entry 1: bonus=3, slot flags = Slashing (0x4000) + bit 0. */
    uint8_t *item2 = catalog.items + 1 * ItemRecordSize;
    setU16At(item2, ItemFieldFlags, ItemFlagEquipCode0C);
    setU16At(item2, ItemFieldTargetOffset, ItemWeaponSize);
    setWord(catalog.weapons, ItemWeaponSize, ItemTargetAbsorption, 3);
    setWord(catalog.weapons, ItemWeaponSize, ItemTargetSlotFlags, 0x4001);

    /* Item 3: ring slot (code 0xD), wearable-kind. Wearable target entry 0: bonus=2. */
    uint8_t *item3 = catalog.items + 2 * ItemRecordSize;
    setU16At(item3, ItemFieldFlags, ItemFlagEquipCode0D);
    setU16At(item3, ItemFieldTargetOffset, 0);
    setWord(catalog.wearables, 0, ItemTargetAbsorption, 2);

    /* Item 4: short slot (code 0x10), wearable-kind. Wearable target entry 1: bonus=1. */
    uint8_t *item4 = catalog.items + 3 * ItemRecordSize;
    setU16At(item4, ItemFieldFlags, ItemFlagEquipShort);
    setU16At(item4, ItemFieldTargetOffset, ItemWearableSize);
    setWord(catalog.wearables, ItemWearableSize, ItemTargetAbsorption, 1);

    memset(g_record, 0, PartyRecordSize);
    partySetU16(g_record, PartyFieldEquipRatingBase1, 10);
    partySetU16(g_record, PartyFieldEquipRatingBase1Max, 10);
    partySetStat(g_record, PartyStatProjectile, 7);
    partySetStatMax(g_record, PartyStatProjectile, 8);
    partySetStat(g_record, PartyStatSlashing, 4);
    partySetStatMax(g_record, PartyStatSlashing, 6);
    itemSlotSet(partyEquipmentSlot(g_record, 0x0A, GameYendor2), 1, 0); /* main weapon: item 1 */
    itemSlotSet(partyEquipmentSlot(g_record, 0x0C, GameYendor2), 2, 0); /* off-hand: item 2 */
    itemSlotSet(partyEquipmentSlot(g_record, 0x0D, GameYendor2), 3, 0); /* ring: item 3 */
    partySetU16(partyEquipmentSlot(g_record, 0x10, GameYendor2), 0, 4); /* short slot: item 4 (2-byte, id only) */

    partyRecomputeEquipmentStatBonuses(g_record, &catalog, GameYendor2);

    checkU32("EquipRating1 = base(10) + Projectile skill(7, from the main weapon)",
             partyGetStat(g_record, PartyStatEquipRating1), 17);
    checkU32("EquipRating1 max = base(10) + max Projectile(8)", partyGetStatMax(g_record, PartyStatEquipRating1), 18);
    checkU32("EquipRating2 = main weapon's own bonus (5), base was 0",
             partyGetStat(g_record, PartyStatEquipRating2), 5);
    checkU32("EquipRating3 = Slashing skill(4, selected by the off-hand's slot-flags bit 0x4000)",
             partyGetStat(g_record, PartyStatEquipRating3), 4);
    checkU32("EquipRating3 max = max Slashing(6)", partyGetStatMax(g_record, PartyStatEquipRating3), 6);
    checkU32("EquipRating4 = off-hand's own bonus (3), base was 0",
             partyGetStat(g_record, PartyStatEquipRating4), 3);
    checkU32("EquipRating5 = ring(2) + short-slot(1) bonuses accumulated", partyGetStat(g_record, PartyStatEquipRating5),
             3);
    check("off-hand slot-flags bit 0 set: PartyFieldUiFlags bit 0x20 is set",
          (partyGetU16(g_record, PartyFieldUiFlags) & 0x20) != 0);

    /* An empty off-hand: no skill/item bonus, and the UI flag stays clear. */
    memset(g_record, 0, PartyRecordSize);
    itemSlotSet(partyEquipmentSlot(g_record, 0x0A, GameYendor2), 1, 0);
    partySetStat(g_record, PartyStatProjectile, 7);
    partyRecomputeEquipmentStatBonuses(g_record, &catalog, GameYendor2);
    checkU32("no off-hand: EquipRating3 stays at its base (0)", partyGetStat(g_record, PartyStatEquipRating3), 0);
    checkU32("no off-hand: EquipRating4 stays at its base (0)", partyGetStat(g_record, PartyStatEquipRating4), 0);
    check("no off-hand: PartyFieldUiFlags bit 0x20 stays clear", (partyGetU16(g_record, PartyFieldUiFlags) & 0x20) == 0);

    /* No weapon at all: EquipRating1/2 stay at their base values. */
    memset(g_record, 0, PartyRecordSize);
    partySetU16(g_record, PartyFieldEquipRatingBase1, 9);
    partyRecomputeEquipmentStatBonuses(g_record, &catalog, GameYendor2);
    checkU32("no weapon: EquipRating1 stays at its base (9), no Projectile added",
             partyGetStat(g_record, PartyStatEquipRating1), 9);
    checkU32("no weapon: EquipRating2 stays at its base (0)", partyGetStat(g_record, PartyStatEquipRating2), 0);
}

static void testAbilityUnlocks(void) {
    uint16_t ids[PartyAbilityUnlockSlotCount];

    /* Row 0 (base 4, MONK-and-any-tier), level 2 (column 0): a single id, 5. */
    checkU32("MONK level 2: one ability unlocked", partyAbilityUnlocksAtLevel(4, 2, GameYendor2, ids), 1);
    checkU32("MONK level 2: ability id 5", ids[0], 5);

    /* Level 4 (column 1): two ids in Chapter 2 (7, 0xb). */
    checkU32("MONK level 4: two abilities unlocked", partyAbilityUnlocksAtLevel(4, 4, GameYendor2, ids), 2);
    checkU32("MONK level 4: first id 7", ids[0], 7);
    checkU32("MONK level 4: second id 0xb", ids[1], 0x0b);

    /* Chapter 3 has different ids at the same (row, level) -- a real per-game content difference. */
    checkU32("MONK level 4, yendor3: different ids (6, 7)", partyAbilityUnlocksAtLevel(4, 4, GameYendor3, ids), 2);
    checkU32("yendor3 first id 6", ids[0], 6);
    checkU32("yendor3 second id 7", ids[1], 7);

    /* Tier doesn't matter for the row -- base 4 at tier 1 (class id 14) or tier 2 (24) gives the same row. */
    uint16_t idsPromoted[PartyAbilityUnlockSlotCount];
    checkU32("class id 14 (tier-1 base-4): same count as tier 0", partyAbilityUnlocksAtLevel(14, 2, GameYendor2, idsPromoted), 1);
    checkU32("class id 14: same ability id 5 as tier 0", idsPromoted[0], 5);
    partyAbilityUnlocksAtLevel(24, 2, GameYendor2, idsPromoted);
    checkU32("class id 24 (tier-2 base-4): same ability id 5 too", idsPromoted[0], 5);

    check("odd level: no abilities", partyAbilityUnlocksAtLevel(4, 3, GameYendor2, ids) == 0);
    check("level 0: no abilities", partyAbilityUnlocksAtLevel(4, 0, GameYendor2, ids) == 0);
    check("level past the table's 40-level end: no abilities", partyAbilityUnlocksAtLevel(4, 42, GameYendor2, ids) == 0);

    /* Physical classes (base 1-3), at ANY tier, never have a valid row -- the deliberately-not-reproduced OOB case. */
    check("FIGHTER (base 1, tier 0): no abilities", partyAbilityUnlocksAtLevel(1, 2, GameYendor2, ids) == 0);
    check("WARRIOR (base 1, tier 1): no abilities", partyAbilityUnlocksAtLevel(11, 2, GameYendor2, ids) == 0);
    check("CHAMPION (base 1, tier 2): no abilities", partyAbilityUnlocksAtLevel(21, 2, GameYendor2, ids) == 0);
    check("ROGUE (base 3): no abilities", partyAbilityUnlocksAtLevel(3, 2, GameYendor2, ids) == 0);

    /* Applying: flag bits actually get set. */
    memset(g_record, 0, PartyRecordSize);
    unsigned count = partyApplyAbilityUnlocks(g_record, 4, 4, GameYendor2);
    checkU32("apply: 2 abilities set at MONK level 4", count, 2);
    check("apply: flag 7 is set", flagBankTest(g_record + PartyFieldFlagBankCA, 16, 7));
    check("apply: flag 0xb is set", flagBankTest(g_record + PartyFieldFlagBankCA, 16, 0x0b));
    check("apply: an unrelated flag stays clear", !flagBankTest(g_record + PartyFieldFlagBankCA, 16, 1));

    /* Wired into partyApplyTraining itself, using the pre-promotion class id, at an even level. */
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    Bcd4 cost;
    bcd4FromU16(cost, 100);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);
    setupTrainee(g_record, 4 /* MONK */, 1, 10, 10, 10, 10, 10, 10, 100, 30); /* level 1 -> 2 */
    partyApplyTraining(g_record, GameYendor2, &save, cost);
    check("partyApplyTraining at level 2 also sets the MONK level-2 ability (id 5)",
          flagBankTest(g_record + PartyFieldFlagBankCA, 16, 5));
}

int main(void) {
    testLayoutRelations();
    testStats();
    testClasses();
    testFlagBanks();
    testInventory();
    testLevelUp();
    testTraining();
    testEquipmentBonuses();
    testAbilityUnlocks();
    testRealCharacters();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

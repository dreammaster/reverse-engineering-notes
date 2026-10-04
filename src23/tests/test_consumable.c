/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_consumable test_consumable.c ../consumable.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_consumable
 */
#include <stdio.h>
#include <string.h>

#include "consumable.h"
#include "party.h"

static int g_failureCount = 0;

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

static void setPoints(uint8_t *r, uint16_t hp, uint16_t hpMax, uint16_t mp, uint16_t mpMax) {
    partySetStat(r, PartyStatHitPoints, hp);
    partySetStatMax(r, PartyStatHitPoints, hpMax);
    partySetStat(r, PartyStatMagicPoints, mp);
    partySetStatMax(r, PartyStatMagicPoints, mpMax);
}

static bool useItem(GameKind game, unsigned id, uint8_t *r) {
    return partyUseRestorative(partyRestorativeForItem(game, id), r);
}

static void testIdMapping(void) {
    check("Chapter 2 ids", partyRestorativeForItem(GameYendor2, 0x12) == RestorativeHpQuarter &&
                               partyRestorativeForItem(GameYendor2, 0x1D) == RestorativeMpHalf &&
                               partyRestorativeForItem(GameYendor2, 0x18) == RestorativeCure);
    check("Chapter 3 ids", partyRestorativeForItem(GameYendor3, 0x34) == RestorativeHpQuarter &&
                               partyRestorativeForItem(GameYendor3, 0x37) == RestorativeMpHalf &&
                               partyRestorativeForItem(GameYendor3, 0x39) == RestorativeCure);
    check("the games' ids do not cross over", partyRestorativeForItem(GameYendor2, 0x34) == RestorativeNone &&
                                                  partyRestorativeForItem(GameYendor3, 0x12) == RestorativeNone);
    check("0x1C (transmutation) is not a restorative", partyRestorativeForItem(GameYendor2, 0x1C) == RestorativeNone);
}

static void testHealthItems(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    setPoints(r, 10, 100, 0, 0);
    check("item 0x12 heals", useItem(GameYendor2, 0x12, r));
    checkU32("a quarter of the maximum", partyGetStat(r, PartyStatHitPoints), 35);
    check("0x13 heals", useItem(GameYendor2, 0x13, r));
    checkU32("half the maximum on top", partyGetStat(r, PartyStatHitPoints), 85);
    check("0x13 again", useItem(GameYendor2, 0x13, r));
    checkU32("is capped at the maximum", partyGetStat(r, PartyStatHitPoints), 100);
    check("a full-health member cannot use any of them",
          !useItem(GameYendor2, 0x12, r) && !useItem(GameYendor2, 0x13, r) && !useItem(GameYendor2, 0x14, r));
    setPoints(r, 1, 77, 0, 0);
    check("0x14 heals fully", useItem(GameYendor2, 0x14, r) && partyGetStat(r, PartyStatHitPoints) == 77);
    setPoints(r, 0, 9, 0, 0);
    useItem(GameYendor2, 0x12, r);
    checkU32("a quarter of 9 truncates to 2", partyGetStat(r, PartyStatHitPoints), 2);
}

static void testMagicItems(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    setPoints(r, 5, 5, 0, 0);
    check("a member with no magic cannot use the MP items",
          !useItem(GameYendor2, 0x17, r) && !useItem(GameYendor2, 0x1D, r));
    setPoints(r, 5, 5, 4, 40);
    check("0x1D restores half the maximum", useItem(GameYendor2, 0x1D, r) && partyGetStat(r, PartyStatMagicPoints) == 24);
    check("0x17 restores fully", useItem(GameYendor2, 0x17, r) && partyGetStat(r, PartyStatMagicPoints) == 40);
    check("...then there is nothing to restore", !useItem(GameYendor2, 0x17, r));
    setPoints(r, 5, 5, 30, 40);
    useItem(GameYendor2, 0x1D, r);
    checkU32("+half is capped at the maximum", partyGetStat(r, PartyStatMagicPoints), 40);
}

static void testCure(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    partySetU16(r, PartyFieldStatusFlags, PartyStatusCursed | PartyStatusDead);
    check("0x18 does not cure a curse or death", !useItem(GameYendor2, 0x18, r));
    partySetU16(r, PartyFieldStatusFlags, PartyStatusSick | PartyStatusPoisoned | PartyStatusCursed | 0x0005);
    check("it cures sick/poisoned/diseased", useItem(GameYendor2, 0x18, r));
    checkU32("...but leaves everything below 0x2000, even Cursed", partyGetU16(r, PartyFieldStatusFlags),
             PartyStatusCursed | 0x0005);
    check("an unknown id does nothing", !useItem(GameYendor2, 0x15, r));
}

static void testAlchemy(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    Bcd4 magic, nuore, expected;
    unsigned consumed, yield;
    bcd4FromU16(magic, 250);
    bcd4FromU16(nuore, 5);

    partySetStat(r, PartyStatChemistry, 64);
    check("Chemistry 64 is not enough", partyTransmuteOre(r, magic, nuore, &consumed, &yield) == AlchemySkillTooLow);
    partySetStat(r, PartyStatChemistry, 65);
    bcd4FromU16(magic, 9);
    check("fewer than 10 units cannot be transmuted", partyTransmuteOre(r, magic, nuore, &consumed, &yield) == AlchemyTooFewUnits);

    bcd4FromU16(magic, 250);
    check("the stack is converted", partyTransmuteOre(r, magic, nuore, &consumed, &yield) == AlchemyDone);
    checkU32("100 units consumed (the cap), not 250", consumed, 100);
    checkU32("at skill 65 the divisor is 10: 10 yielded", yield, 10);
    bcd4FromU16(expected, 150);
    check("the source lost 100", bcd4Compare(magic, expected) == 0);
    bcd4FromU16(expected, 15);
    check("the destination gained 10", bcd4Compare(nuore, expected) == 0);

    bcd4FromU16(magic, 37);
    partySetStat(r, PartyStatChemistry, 95);
    partyTransmuteOre(r, magic, nuore, &consumed, &yield);
    checkU32("a small stack goes entirely: 37", consumed, 37);
    checkU32("skill 95 divides by 4: 9", yield, 9);
    bcd4FromU16(expected, 0);
    check("the source is emptied", bcd4Compare(magic, expected) == 0);

    checkU32("divisor below 80", alchemyYieldDivisor(79), 10);
    checkU32("divisor 80", alchemyYieldDivisor(80), 5);
    checkU32("divisor 94", alchemyYieldDivisor(94), 5);
    checkU32("divisor 109", alchemyYieldDivisor(109), 4);
    checkU32("divisor 110", alchemyYieldDivisor(110), 2);
}

static void testPercent(void) {
    check("Chapter 2 percentage items are 0x36-0x46, Chapter 3 0x1F-0x20", partyIsPercentRestorativeItem(GameYendor2, 0x36) && partyIsPercentRestorativeItem(GameYendor2, 0x46) &&
                                                                              !partyIsPercentRestorativeItem(GameYendor2, 0x47) && !partyIsPercentRestorativeItem(GameYendor2, 0x35) &&
                                                                              partyIsPercentRestorativeItem(GameYendor3, 0x1F) && partyIsPercentRestorativeItem(GameYendor3, 0x20) &&
                                                                              !partyIsPercentRestorativeItem(GameYendor3, 0x21));
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    setPoints(r, 10, 200, 5, 80);
    partySetU16(r, PartyFieldClass, 1);
    check("health 25%: 10 + (200 x 25 + 50) / 100 = 60", partyUsePercentRestorative(GameYendor2, r, false, 25) == PercentRestoreApplied && partyGetStat(r, PartyStatHitPoints) == 60);
    partyUsePercentRestorative(GameYendor2, r, false, 100);
    check("health is capped at the maximum", partyGetStat(r, PartyStatHitPoints) == 200);
    check("a fighter (class 1) using a magic item gets nothing and is made sick", partyUsePercentRestorative(GameYendor2, r, true, 50) == PercentRestoreMadeSick &&
                                                                                     partyGetStat(r, PartyStatMagicPoints) == 5 && (partyGetU16(r, PartyFieldStatusFlags) & PartyStatusSick));
    partySetU16(r, PartyFieldStatusFlags, 0);
    partySetU16(r, PartyFieldClass, 4);
    check("a class 4 caster gains 50% of 80: 5 + 40", partyUsePercentRestorative(GameYendor2, r, true, 50) == PercentRestoreApplied && partyGetStat(r, PartyStatMagicPoints) == 45 &&
                                                         partyGetU16(r, PartyFieldStatusFlags) == 0);
    partySetU16(r, PartyFieldClass, 17);
    partySetStat(r, PartyStatMagicPoints, 0);
    check("class ids above 9 reduce by 10 (17 -> 7, a caster)", partyUsePercentRestorative(GameYendor2, r, true, 10) == PercentRestoreApplied && partyGetStat(r, PartyStatMagicPoints) == 8);
    partySetU16(r, PartyFieldClass, 23);
    check("...and again (23 -> 13 -> 3, not a caster)", partyUsePercentRestorative(GameYendor2, r, true, 10) == PercentRestoreMadeSick);
    partySetU16(r, PartyFieldClass, 4);
    partySetStat(r, PartyStatHitPoints, 1);
    partySetStat(r, PartyStatMagicPoints, 0);
    partyUsePercentRestorative(GameYendor3, r, false, 50);
    check("Chapter 3 always restores magic whatever the flag", partyGetStat(r, PartyStatMagicPoints) == 40 && partyGetStat(r, PartyStatHitPoints) == 1);
    setPoints(r, 0, 9999, 0, 80);
    partyUsePercentRestorative(GameYendor2, r, false, 100);
    check("9999 x 100 uses the full 32-bit product: 9999", partyGetStat(r, PartyStatHitPoints) == 9999);
    setPoints(r, 0, 655, 0, 80);
    partyUsePercentRestorative(GameYendor2, r, false, 100); /* 65500 + 50 = 65550 carries out of the low word: the carry is lost (original quirk) */
    check("a carry out of the low word is dropped like the original: (65500 + 50) mod 65536 = 14 -> 0", partyGetStat(r, PartyStatHitPoints) == 0);
}

int main(void) {
    testPercent();
    testIdMapping();
    testHealthItems();
    testMagicItems();
    testCure();
    testAlchemy();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

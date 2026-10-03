/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_thrown test_thrown.c ../thrown.c ../monster.c ../random.c ../item.c && ./test_thrown
 */
#include <stdio.h>
#include <string.h>

#include "monster.h"
#include "thrown.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

/* A seed whose first roll is <= 85 (and whose second roll is <= 70 / > 70 as asked). */
static RandomState findRng(bool firstOk, int secondMode) {
    for (uint8_t s = 0; s < 250; s++) {
        RandomState rng;
        randomStart(&rng, s, (uint8_t)(s * 7));
        RandomState peek = rng;
        unsigned a = randomInRange(&peek, 100), b = randomInRange(&peek, 100);
        if ((a <= 85) != firstOk) {
            continue;
        }
        if (firstOk && secondMode == 1 && b > 70) {
            continue;
        }
        if (firstOk && secondMode == 2 && b <= 70) {
            continue;
        }
        return rng;
    }
    RandomState none;
    randomStart(&none, 0, 0);
    return none;
}

static void testKinds(void) {
    check("Chapter 2 ids", thrownKindForItem(GameYendor2, 0x19) == ThrownGoldPotion && thrownKindForItem(GameYendor2, 0x1A) == ThrownSilverPotion &&
                               thrownKindForItem(GameYendor2, 0x1B) == ThrownBluePotion && thrownKindForItem(GameYendor2, 0x240) == ThrownFlamingOil);
    check("Chapter 3 ids", thrownKindForItem(GameYendor3, 0x3D) == ThrownGoldPotion && thrownKindForItem(GameYendor3, 0x3E) == ThrownSilverPotion &&
                               thrownKindForItem(GameYendor3, 0x3F) == ThrownBluePotion && thrownKindForItem(GameYendor3, 0x3C) == ThrownFlamingOil);
    check("they do not cross over", thrownKindForItem(GameYendor2, 0x3D) == ThrownNone && thrownKindForItem(GameYendor3, 0x19) == ThrownNone);
}

static void testResolve(void) {
    uint8_t mon[MonsterRecordSize];
    memset(mon, 0, sizeof(mon));
    monsterSetU16(mon, MonsterFieldTickAmount, 9);
    monsterSetU16(mon, MonsterFieldTickCountdown, 9);
    RandomState rng = findRng(false, 0);
    ThrownEffect e = thrownResolveAbilityEffect(GameYendor2, 0x19, mon, false, &rng);
    check("a roll above 85 fizzles, but the timers were already cleared", e.damage == 0 && e.typeFlags == 0 &&
                                                                          monsterGetU16(mon, MonsterFieldTickAmount) == 0 &&
                                                                          monsterGetU16(mon, MonsterFieldTickCountdown) == 0);
    rng = findRng(true, 0);
    e = thrownResolveAbilityEffect(GameYendor2, 0x19, mon, false, &rng);
    check("GOLD POTION: 60 physical", e.damage == 60 && e.typeFlags == 0x8000 && e.statusFlags == 0 && !e.corridor);

    rng = findRng(true, 1);
    e = thrownResolveAbilityEffect(GameYendor2, 0x1A, mon, false, &rng);
    check("SILVER POTION: 35 and poison, timers 6 x 5", e.damage == 35 && e.statusFlags == 0x8000 && monsterGetU16(mon, MonsterFieldTickAmount) == 6 &&
                                                         monsterGetU16(mon, MonsterFieldTickCountdown) == 5);
    rng = findRng(true, 2);
    e = thrownResolveAbilityEffect(GameYendor2, 0x1A, mon, false, &rng);
    check("...the status is dropped by a high second roll, the timers stay", e.damage == 35 && e.statusFlags == 0 && monsterGetU16(mon, MonsterFieldTickAmount) == 6);
    monsterSetU16(mon, MonsterFieldImmunities, MonsterImmunePoison);
    rng = findRng(true, 1);
    e = thrownResolveAbilityEffect(GameYendor2, 0x1A, mon, false, &rng);
    check("a poison-immune monster takes the damage only, with no timers", e.damage == 35 && e.statusFlags == 0 && monsterGetU16(mon, MonsterFieldTickAmount) == 0);
    monsterSetU16(mon, MonsterFieldImmunities, 0);

    rng = findRng(true, 0);
    e = thrownResolveAbilityEffect(GameYendor2, 0x1B, mon, false, &rng);
    check("BLUE POTION does nothing to the living", e.damage == 0 && e.typeFlags == 0x8000);
    monsterSetU16(mon, MonsterFieldUnknown4E, 13);
    e = thrownResolveAbilityEffect(GameYendor2, 0x1B, mon, false, &rng);
    check("...50 to the undead (85 in Chapter 3)", e.damage == 50 && thrownResolveAbilityEffect(GameYendor3, 0x3F, mon, false, &rng).damage == 85);
    monsterSetU16(mon, MonsterFieldResistances, 0x0200);
    check("...unless they resist 0x200", thrownResolveAbilityEffect(GameYendor2, 0x1B, mon, false, &rng).damage == 0);
    monsterSetU16(mon, MonsterFieldResistances, 0);

    e = thrownResolveAbilityEffect(GameYendor2, 0x240, mon, false, &rng);
    check("FLAMING OIL: 40 along the corridor", e.damage == 40 && e.corridor);
    e = thrownResolveAbilityEffect(GameYendor2, 0x240, mon, true, &rng);
    check("...nothing in formal combat, not even the type flag", e.damage == 0 && e.typeFlags == 0 && !e.corridor);
}

static void testApply(void) {
    uint8_t mon[MonsterRecordSize];
    memset(mon, 0, sizeof(mon));
    monsterSetU16(mon, MonsterFieldHealth, 100);
    ThrownEffect none = {0, 0x8000, 0x8000, false};
    check("zero damage does nothing at all", !thrownApplyResolvedDamage(mon, &none) && monsterGetU16(mon, MonsterFieldState) == 0);

    ThrownEffect hit = {60, 0x8000, 0x8000, false};
    check("a hit subtracts the damage", thrownApplyResolvedDamage(mon, &hit) && monsterGetU16(mon, MonsterFieldHealth) == 40);
    check("...marks the monster hit and applies the status", (monsterGetU16(mon, MonsterFieldState) & 3) == 3 &&
                                                              (monsterGetU16(mon, MonsterFieldState) & 0x8000));
    monsterSetU16(mon, MonsterFieldState, 0);
    monsterSetU16(mon, MonsterFieldImmunities, 0x8000);
    thrownApplyResolvedDamage(mon, &hit);
    check("immune statuses are not applied", (monsterGetU16(mon, MonsterFieldState) & 0x8000) == 0 && monsterGetU16(mon, MonsterFieldHealth) == 0);

    monsterSetU16(mon, MonsterFieldHealth, 100);
    monsterSetU16(mon, MonsterFieldResistances, 0x8000);
    thrownApplyResolvedDamage(mon, &hit);
    check("one overlapping resistance bit halves the damage", monsterGetU16(mon, MonsterFieldHealth) == 70);
    monsterSetU16(mon, MonsterFieldHealth, 100);
    monsterSetU16(mon, MonsterFieldResistances, 0xC000);
    ThrownEffect two = {60, 0xC000, 0, false};
    thrownApplyResolvedDamage(mon, &two);
    check("two bits quarter it", monsterGetU16(mon, MonsterFieldHealth) == 85);
    monsterSetU16(mon, MonsterFieldHealth, 100);
    monsterSetU16(mon, MonsterFieldResistances, 0x4000);
    thrownApplyResolvedDamage(mon, &hit);
    check("a resistance the attack does not carry changes nothing", monsterGetU16(mon, MonsterFieldHealth) == 40);
}

static void testWeaponAndRows(void) {
    check("a plain ranged weapon is physical only", thrownWeaponTypeFlags(0, 0) == 0x8000);
    check("word 1 bit 0x400 adds 0x1000, a non-zero word 4 adds 0x800", thrownWeaponTypeFlags(0x400, 0) == 0x9000 && thrownWeaponTypeFlags(0, 7) == 0x8800 &&
                                                                         thrownWeaponTypeFlags(0x400, 7) == 0x9800);
    unsigned s[3];
    thrownCorridorRowStarts(0x24, s);
    check("depth 0x24", s[0] == 0x18 && s[1] == 0x23 && s[2] == 0x27);
    thrownCorridorRowStarts(0x28, s);
    check("depth 0x28", s[0] == 0x23 && s[1] == 0x27 && s[2] == 0x2A);
    thrownCorridorRowStarts(0x2B, s);
    check("depth 0x2B", s[0] == 0x27 && s[1] == 0x2A && s[2] == 0x2D);
    thrownCorridorRowStarts(0x2E, s);
    check("anything else", s[0] == 0x2A && s[1] == 0x2D && s[2] == 0x30);
}

int main(void) {
    testKinds();
    testResolve();
    testApply();
    testWeaponAndRows();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_repair test_repair.c ../repair.c ../random.c && ./test_repair
 */
#include <stdio.h>

#include "repair.h"

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

static void testTiers(void) {
    checkU32("49 is tier 0", repairSkillTier(49), 0);
    checkU32("50 is tier 1", repairSkillTier(50), 1);
    checkU32("64 is tier 1", repairSkillTier(64), 1);
    checkU32("65 is tier 2", repairSkillTier(65), 2);
    checkU32("79 is tier 2", repairSkillTier(79), 2);
    checkU32("80 is tier 3", repairSkillTier(80), 3);
    checkU32("94 is tier 3", repairSkillTier(94), 3);
    checkU32("95 is tier 4", repairSkillTier(95), 4);
    checkU32("999 is tier 4", repairSkillTier(999), 4);
    checkU32("a negative stat is tier 0 (signed)", repairSkillTier((uint16_t)-1), 0);
}

static void testTable(void) {
    uint16_t low, high;
    check("row 0 tier 0", repairThresholds(0, 0, &low, &high) && low == 20 && high == 50);
    check("row 4 tier 0 destroys on anything below 100", repairThresholds(4, 0, &low, &high) && low == 100 && high == 0);
    check("row 0 tier 4 always succeeds", repairThresholds(0, 4, &low, &high) && low == 0 && high == 100);
    check("row 15 tier 4", repairThresholds(15, 4, &low, &high) && low == 10 && high == 80);
    check("row 16 is past the table", !repairThresholds(16, 0, &low, &high));
    check("tier 5 is past the table", !repairThresholds(0, 5, &low, &high));

    /* every row: high-skill thresholds never ask for more failures than the next-lower tier */
    bool monotone = true;
    for (unsigned row = 0; row < RepairRows; row++) {
        for (unsigned tier = 1; tier < RepairTiers; tier++) {
            uint16_t lowA, highA, lowB, highB;
            repairThresholds(row, tier - 1, &lowA, &highA);
            repairThresholds(row, tier, &lowB, &highB);
            if (lowB > lowA) {
                monotone = false; /* a better repairer must never have a higher destroy threshold */
            }
        }
    }
    check("a better tier never has a higher critical-failure threshold", monotone);
}

static void testAttemptMatchesTheRoll(void) {
    for (uint8_t seed = 1; seed < 30; seed++) {
        for (unsigned row = 0; row < 6; row++) {
            for (uint16_t skill = 0; skill <= 100; skill += 25) {
                RandomState rng, peek;
                randomStart(&rng, seed, seed);
                peek = rng;
                int16_t roll = (int16_t)randomInRange(&peek, 100);
                uint16_t low, high;
                repairThresholds(row, repairSkillTier(skill), &low, &high);
                RepairOutcome expected = roll < (int16_t)low ? RepairCriticalFail : roll <= (int16_t)high ? RepairSuccess : RepairSoftFail;
                if (repairAttempt(row, skill, &rng) != expected) {
                    check("repairAttempt matches an independent roll", false);
                    return;
                }
            }
        }
    }
    check("repairAttempt matches an independent roll for 29 seeds x 6 rows x 5 skills", true);
}

static void testOutOfTableRowDoesNotRoll(void) {
    RandomState rng, before;
    randomStart(&rng, 3, 3);
    before = rng;
    check("a row past the table is a soft failure", repairAttempt(16, 100, &rng) == RepairSoftFail);
    check("...and consumes no random draw", rng.seed == before.seed && rng.clockWord == before.clockWord);
}

static void testCharge(void) {
    check("a soft failure keeps the kit's charge", !repairConsumesCharge(RepairSoftFail));
    check("a critical failure spends it", repairConsumesCharge(RepairCriticalFail));
    check("a success spends it", repairConsumesCharge(RepairSuccess));
}

int main(void) {
    testTiers();
    testTable();
    testAttemptMatchesTheRoll();
    testOutOfTableRowDoesNotRoll();
    testCharge();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

#include "repair.h"

/* RepairItemCommand's table: per row, five (low, high) pairs by skill tier. */
static const uint16_t g_table[RepairRows][RepairTiers][2] = {
    {{20, 50}, {5, 65}, {5, 85}, {0, 95}, {0, 100}},    {{60, 40}, {10, 65}, {5, 85}, {0, 95}, {0, 100}},
    {{80, 10}, {15, 50}, {5, 85}, {0, 90}, {0, 100}},   {{90, 10}, {20, 50}, {5, 85}, {0, 90}, {0, 100}},
    {{100, 0}, {20, 50}, {10, 75}, {5, 80}, {0, 100}},  {{100, 0}, {30, 45}, {10, 75}, {5, 80}, {0, 100}},
    {{100, 0}, {40, 45}, {10, 75}, {5, 80}, {0, 100}},  {{100, 0}, {40, 40}, {20, 75}, {10, 80}, {0, 100}},
    {{100, 0}, {60, 30}, {20, 65}, {10, 80}, {0, 100}}, {{100, 0}, {60, 30}, {20, 65}, {15, 70}, {0, 95}},
    {{100, 0}, {75, 20}, {30, 65}, {15, 70}, {0, 95}},  {{100, 0}, {75, 20}, {30, 50}, {15, 70}, {0, 95}},
    {{100, 0}, {85, 10}, {30, 50}, {20, 60}, {5, 90}},  {{100, 0}, {90, 5}, {40, 30}, {20, 60}, {5, 90}},
    {{100, 0}, {90, 5}, {40, 30}, {30, 50}, {10, 85}},  {{100, 0}, {90, 5}, {40, 30}, {30, 50}, {10, 80}},
};

unsigned repairSkillTier(uint16_t repairStat) {
    int16_t skill = (int16_t)repairStat;
    if (skill < 0x32) {
        return 0;
    }
    if (skill < 0x41) {
        return 1;
    }
    if (skill < 0x50) {
        return 2;
    }
    if (skill < 0x5F) {
        return 3;
    }
    return 4;
}

bool repairThresholds(unsigned row, unsigned tier, uint16_t *low, uint16_t *high) {
    if (row >= RepairRows || tier >= RepairTiers) {
        return false;
    }
    *low = g_table[row][tier][0];
    *high = g_table[row][tier][1];
    return true;
}

RepairOutcome repairAttempt(unsigned row, uint16_t repairStat, RandomState *rng) {
    uint16_t low, high;
    if (!repairThresholds(row, repairSkillTier(repairStat), &low, &high)) {
        return RepairSoftFail;
    }
    int16_t roll = (int16_t)randomInRange(rng, 100);
    if (roll < (int16_t)low) {
        return RepairCriticalFail;
    }
    if (roll <= (int16_t)high) {
        return RepairSuccess;
    }
    return RepairSoftFail;
}

bool repairConsumesCharge(RepairOutcome outcome) {
    return outcome != RepairSoftFail;
}

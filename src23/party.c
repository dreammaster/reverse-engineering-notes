#include "party.h"

#include <string.h>

#include "bcd4.h"

/* Names from the game's 27-entry table at DS:0x7DC7; NULL where the table entry is blank. */
static const char *const g_statNames[PartyStatCount] = {
    "STRENGTH", "DEXTERITY", "STAMINA", "INTELLIGENCE", "WISDOM", "CHARISMA",
    NULL, NULL, NULL, NULL, NULL,
    "HIT POINTS", "MAGIC POINTS", NULL,
    "SURVIVAL", "PROJECTILE", "SLASHING", "BASHING", "POLEARM", "CASTING", "MAPPING",
    "NAVIGATION", "BARTERING", "REPAIR", "THIEVERY", "LINGUISTICS", "CHEMISTRY",
};

/* The three 9-entry tables at DS:0x7982, 0x8434 and 0x8497. */
static const char *const g_classNames[3][9] = {
    {"FIGHTER", "MERCHANT", "ROGUE", "MONK", "ALCHEMIST", "PALADIN", "MAGE", "DRUID", "MARKSMAN"},
    {"WARRIOR", "TINKERER", "THIEF", "CLERIC", "TRANSMUTER", "CAVALIER", "WIZARD", "ENCHANTER", "RANGER"},
    {"CHAMPION", "BLACKSMITH", "ASSASSIN", "PRIEST", "HEALER", "HERO", "SORCERER", "SAGE", "KNIGHT"},
};

uint16_t partyGetU16(const uint8_t *record, unsigned offset) {
    return (uint16_t)(record[offset] | (record[offset + 1] << 8));
}

void partySetU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

void partyGetName(const uint8_t *record, char out[PartyNameBufferSize]) {
    size_t length = 0;
    while (length < PartyNameMaxLength && record[PartyFieldName + length] != 0) {
        length++;
    }
    memcpy(out, record + PartyFieldName, length);
    out[length] = '\0';
}

uint16_t partyGetStat(const uint8_t *record, PartyStat stat) {
    if ((unsigned)stat >= PartyStatCount) {
        return 0;
    }
    return partyGetU16(record, PartyFieldStats + stat * 2);
}

uint16_t partyGetStatMax(const uint8_t *record, PartyStat stat) {
    if ((unsigned)stat >= PartyStatCount) {
        return 0;
    }
    return partyGetU16(record, PartyFieldStatsMax + stat * 2);
}

void partySetStat(uint8_t *record, PartyStat stat, uint16_t value) {
    if ((unsigned)stat < PartyStatCount) {
        partySetU16(record, PartyFieldStats + stat * 2, value);
    }
}

void partySetStatMax(uint8_t *record, PartyStat stat, uint16_t value) {
    if ((unsigned)stat < PartyStatCount) {
        partySetU16(record, PartyFieldStatsMax + stat * 2, value);
    }
}

const char *partyStatName(PartyStat stat, GameKind game) {
    if ((unsigned)stat >= PartyStatCount) {
        return NULL;
    }
    if (game == GameYendor3) {
        if (stat == PartyStatHitPoints) {
            return "HEALTH";
        }
        if (stat == PartyStatChemistry) {
            return NULL;
        }
    }
    return g_statNames[stat];
}

uint16_t partyGetProtection(const uint8_t *record, PartyProtection protection) {
    if ((unsigned)protection >= PartyProtectionCount) {
        return 0;
    }
    return partyGetU16(record, PartyFieldProtections + protection * 2);
}

uint8_t *partyExperience(uint8_t *record) {
    return record + PartyFieldExperience;
}

/* DS:0x9277 (yendor2.asm:20049), extracted via ida_scripts/dump_xp_threshold_table.py. */
static const uint8_t g_xpThresholdsYendor2[PartyXpThresholdCount][4] = {
    {0x00, 0x00, 0x06, 0x80}, {0x00, 0x00, 0x18, 0x50}, {0x00, 0x00, 0x26, 0x00}, {0x00, 0x00, 0x55, 0x00},
    {0x00, 0x01, 0x00, 0x00}, {0x00, 0x01, 0x70, 0x00}, {0x00, 0x02, 0x60, 0x00}, {0x00, 0x04, 0x95, 0x00},
    {0x00, 0x07, 0x97, 0x00}, {0x00, 0x12, 0x50, 0x00}, {0x00, 0x21, 0x90, 0x00}, {0x00, 0x34, 0x00, 0x00},
    {0x00, 0x46, 0x00, 0x00}, {0x00, 0x66, 0x00, 0x00}, {0x00, 0x80, 0x00, 0x00}, {0x00, 0x92, 0x00, 0x00},
    {0x01, 0x10, 0x00, 0x00}, {0x01, 0x21, 0x00, 0x00}, {0x01, 0x35, 0x00, 0x00}, {0x01, 0x64, 0x00, 0x00},
    {0x01, 0x82, 0x00, 0x00}, {0x01, 0x96, 0x00, 0x00}, {0x02, 0x09, 0x00, 0x00}, {0x02, 0x35, 0x00, 0x00},
    {0x02, 0x62, 0x00, 0x00}, {0x03, 0x05, 0x00, 0x00}, {0x03, 0x42, 0x00, 0x00}, {0x03, 0x67, 0x00, 0x00},
    {0x03, 0x85, 0x00, 0x00}, {0x04, 0x27, 0x00, 0x00}, {0x04, 0x49, 0x00, 0x00}, {0x04, 0x75, 0x00, 0x00},
    {0x05, 0x17, 0x00, 0x00}, {0x05, 0x60, 0x00, 0x00}, {0x05, 0x92, 0x00, 0x00}, {0x06, 0x24, 0x00, 0x00},
    {0x06, 0x52, 0x00, 0x00}, {0x07, 0x64, 0x00, 0x00}, {0x37, 0x55, 0x00, 0x00},
    /* Levels 40-89: an effectively-unreachable-via-XP sentinel, 50 identical entries. */
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
    {0x90, 0x00, 0x00, 0x00}, {0x90, 0x00, 0x00, 0x00},
};

/* DS:0xC75F (yendor3.asm:12038), extracted the same way. Same shape (39 real jumps, then a sentinel), different values throughout -- a real Ch2/Ch3 content difference, not shared data. */
static const uint8_t g_xpThresholdsYendor3[PartyXpThresholdCount][4] = {
    {0x00, 0x00, 0x06, 0x80}, {0x00, 0x00, 0x18, 0x00}, {0x00, 0x00, 0x32, 0x00}, {0x00, 0x00, 0x65, 0x00},
    {0x00, 0x01, 0x03, 0x00}, {0x00, 0x01, 0x50, 0x00}, {0x00, 0x02, 0x30, 0x00}, {0x00, 0x03, 0x35, 0x00},
    {0x00, 0x04, 0x20, 0x00}, {0x00, 0x05, 0x19, 0x00}, {0x00, 0x06, 0x58, 0x00}, {0x00, 0x08, 0x55, 0x00},
    {0x00, 0x09, 0x96, 0x00}, {0x00, 0x12, 0x50, 0x00}, {0x00, 0x15, 0x16, 0x00}, {0x00, 0x18, 0x27, 0x00},
    {0x00, 0x23, 0x56, 0x00}, {0x00, 0x28, 0x23, 0x00}, {0x00, 0x37, 0x10, 0x00}, {0x00, 0x48, 0x70, 0x00},
    {0x00, 0x61, 0x10, 0x00}, {0x00, 0x73, 0x00, 0x00}, {0x00, 0x86, 0x60, 0x00}, {0x01, 0x01, 0x00, 0x00},
    {0x01, 0x18, 0x50, 0x00}, {0x01, 0x35, 0x00, 0x00}, {0x01, 0x51, 0x00, 0x00}, {0x01, 0x67, 0x00, 0x00},
    {0x01, 0x85, 0x00, 0x00}, {0x02, 0x22, 0x50, 0x00}, {0x02, 0x60, 0x00, 0x00}, {0x03, 0x16, 0x00, 0x00},
    {0x03, 0x81, 0x00, 0x00}, {0x04, 0x78, 0x00, 0x00}, {0x05, 0x70, 0x00, 0x00}, {0x06, 0x98, 0x00, 0x00},
    {0x08, 0x17, 0x40, 0x00}, {0x09, 0x61, 0x50, 0x00}, {0x10, 0x80, 0x00, 0x00},
    /* Levels 40-89: an effectively-unreachable-via-XP sentinel (99,999,999, not Chapter 2's 90,000,000). */
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
    {0x99, 0x99, 0x99, 0x99}, {0x99, 0x99, 0x99, 0x99},
};

const uint8_t (*partyXpThresholdTable(GameKind game))[4] {
    return game == GameYendor2 ? g_xpThresholdsYendor2 : g_xpThresholdsYendor3;
}

bool partyCheckForLevelUp(uint8_t *record, GameKind game) {
    partySetU16(record, PartyFieldPendingLevel, 0);
    if (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated) {
        return false;
    }

    unsigned level = partyGetU16(record, PartyFieldLevel);
    /*
     * The original indexes the table at [level-1] unconditionally, with
     * no bounds check on this first access (only the cascade loop below
     * checks its own bound) -- a level-90 character (the max, per
     * PartyFieldLevel's own doc) would read 4 bytes past the table.
     * Guarded here instead of reproduced, since it's undefined behavior
     * in C and status quo level-90 characters have nowhere further to
     * advance regardless.
     */
    if (level == 0 || level > PartyXpThresholdCount) {
        return false;
    }

    /*
     * Faithful to a real asymmetry in the original: the first threshold
     * check only requires experience >= table[level-1] to advance once,
     * but each further cascade step requires experience > table[index]
     * (strictly greater) to keep advancing -- landing exactly on a later
     * threshold stops the cascade one level short of where a >= test
     * would put it. Not "fixed" here; reproduced as read.
     */
    const uint8_t(*table)[4] = partyXpThresholdTable(game);
    const uint8_t *experience = partyExperience(record);
    unsigned index = level - 1;
    if (bcd4Compare(experience, table[index]) < 0) {
        return false; /* pendingLevel already 0 */
    }
    unsigned newLevel = level + 1;
    while (index + 1 < PartyXpThresholdCount) {
        index++;
        if (bcd4Compare(experience, table[index]) <= 0) {
            break;
        }
        newLevel++;
    }

    partySetU16(record, PartyFieldPendingLevel, (uint16_t)newLevel);
    return true;
}

const PartyClassPromotionThresholds *partyClassPromotionThresholds(GameKind game) {
    static const PartyClassPromotionThresholds kYendor2 = {10, 30};
    static const PartyClassPromotionThresholds kYendor3 = {0, 0};
    return game == GameYendor2 ? &kYendor2 : &kYendor3;
}

static int partyScalePercentRounded(int value, int percent) {
    return (value * percent + 50) / 100;
}

/* AddToStatCapped (yendor2.asm:18919): cap 9999 for HP/MP max, 999 for everything else; a stat
   that's currently 0 (untrained/inapplicable) is left untouched rather than grown from zero. */
static void trainingAddStatMaxCapped(uint8_t *record, PartyStat stat, int delta) {
    uint16_t current = partyGetStatMax(record, stat);
    if (current == 0) {
        return;
    }
    int cap = (stat == PartyStatHitPoints || stat == PartyStatMagicPoints) ? 9999 : 999;
    int updated = (int)current + delta;
    if (updated > cap) {
        updated = cap;
    }
    partySetStatMax(record, stat, (uint16_t)updated);
}

PartyTrainOutcome partyApplyTraining(uint8_t *record, GameKind game, SaveGame *save, const Bcd4 cost) {
    uint8_t *gold = saveHeaderBcd4(save, SaveHeaderGold);
    if (bcd4Compare(gold, cost) < 0) {
        return PartyTrainOutcomeInsufficientGold;
    }
    bcd4Sub(gold, cost);

    unsigned level = (unsigned)partyGetU16(record, PartyFieldLevel) + 1;
    if (level > 90) {
        level = 90;
    }
    partySetU16(record, PartyFieldLevel, (uint16_t)level);

    /* Max HP grows by 30% of max Stamina; current HP is set to the new max (a full heal). */
    trainingAddStatMaxCapped(record, PartyStatHitPoints,
                              partyScalePercentRounded(partyGetStatMax(record, PartyStatStamina), 30));
    partySetStat(record, PartyStatHitPoints, partyGetStatMax(record, PartyStatHitPoints));

    /*
     * Max MP grows by a class-base-dependent weighting of max Wisdom/Intelligence, scaled
     * 30% -- read directly from the real branch structure (yendor2.asm:21597-21679), not
     * guessed. Bases 1-3 (FIGHTER/MERCHANT/ROGUE, and their tier-1/2 promotions, which share
     * the same base) get no MP growth at all -- not even a zero-delta call.
     */
    unsigned classBase = partyClassBase(partyGetU16(record, PartyFieldClass));
    if (classBase >= 4) {
        int wisdom = partyGetStatMax(record, PartyStatWisdom);
        int intelligence = partyGetStatMax(record, PartyStatIntelligence);
        int mpRaw;
        switch (classBase) {
        case 4: /* MONK */
            mpRaw = wisdom;
            break;
        case 5: /* ALCHEMIST */
            mpRaw = partyScalePercentRounded(wisdom, 75) + partyScalePercentRounded(intelligence, 25);
            break;
        case 6: /* PALADIN */
            mpRaw = partyScalePercentRounded(wisdom, 50);
            break;
        case 8: /* DRUID */
            mpRaw = partyScalePercentRounded(intelligence, 75) + partyScalePercentRounded(wisdom, 25);
            break;
        case 9: /* MARKSMAN */
            mpRaw = partyScalePercentRounded(intelligence, 50);
            break;
        default: /* MAGE (7), and any other/unmatched base */
            mpRaw = intelligence;
            break;
        }
        trainingAddStatMaxCapped(record, PartyStatMagicPoints, partyScalePercentRounded(mpRaw, 30));
        partySetStat(record, PartyStatMagicPoints, partyGetStatMax(record, PartyStatMagicPoints));
    }

    /* Every core attribute and skill grows by a flat +2 (max only), regardless of class. */
    for (PartyStat stat = PartyStatStrength; stat <= PartyStatCharisma; stat++) {
        trainingAddStatMaxCapped(record, stat, 2);
    }
    for (PartyStat stat = PartyStatSurvival; stat <= PartyStatChemistry; stat++) {
        trainingAddStatMaxCapped(record, stat, 2);
    }

    partyApplyAbilityUnlocks(record, partyGetU16(record, PartyFieldClass), level, game);

    const PartyClassPromotionThresholds *thresholds = partyClassPromotionThresholds(game);
    if (level == thresholds->tier1At || level == thresholds->tier2At) {
        partySetU16(record, PartyFieldClass, (uint16_t)(partyGetU16(record, PartyFieldClass) + 10));
    }

    partySyncStagedStats(record);
    partyRefreshCarryCapacityAndAttributeBonuses(record);
    return PartyTrainOutcomeApplied;
}

/* 20% of however far value is past 72, or 0 if it isn't. */
static uint16_t partyExcessBonus(uint16_t value) {
    if (value <= 72) {
        return 0;
    }
    return (uint16_t)partyScalePercentRounded(value - 72, 20);
}

void partyRefreshCarryCapacityAndAttributeBonuses(uint8_t *record) {
    partySetStat(record, PartyStatCarryCapacity, (uint16_t)(10 * partyGetStat(record, PartyStatStrength)));
    partySetStatMax(record, PartyStatCarryCapacity, (uint16_t)(10 * partyGetStatMax(record, PartyStatStrength)));

    partySetU16(record, PartyFieldStrengthBonus, partyExcessBonus(partyGetStat(record, PartyStatStrength)));
    partySetU16(record, PartyFieldDexterityBonus, partyExcessBonus(partyGetStat(record, PartyStatDexterity)));
    partySetU16(record, PartyFieldStrengthBonusMax, partyExcessBonus(partyGetStatMax(record, PartyStatStrength)));
    partySetU16(record, PartyFieldDexterityBonusMax, partyExcessBonus(partyGetStatMax(record, PartyStatDexterity)));
}

void partySyncStagedStats(uint8_t *record) {
    for (PartyStat stat = PartyStatStrength; stat <= PartyStatEquipRating5; stat++) {
        partySetStat(record, stat, partyGetStatMax(record, stat));
    }
    for (PartyStat stat = PartyStatCarryCapacity; stat <= PartyStatChemistry; stat++) {
        partySetStat(record, stat, partyGetStatMax(record, stat));
    }
}

static const uint8_t *equipTargetEntry(const ItemCatalog *catalog, uint8_t *record, unsigned code, GameKind game) {
    uint8_t *slot = partyEquipmentSlot(record, code, game);
    if (!slot) {
        return NULL;
    }
    uint16_t id = itemSlotId(slot);
    if (id == 0) {
        return NULL;
    }
    const uint8_t *item = itemCatalogRecord(catalog, id);
    if (!item) {
        return NULL;
    }
    return itemTargetEntry(catalog, item);
}

static void equipRatingAdd(uint8_t *record, PartyStat stat, uint16_t deltaCurrent, uint16_t deltaMax) {
    partySetStat(record, stat, (uint16_t)(partyGetStat(record, stat) + deltaCurrent));
    partySetStatMax(record, stat, (uint16_t)(partyGetStatMax(record, stat) + deltaMax));
}

void partyRecomputeEquipmentStatBonuses(uint8_t *record, const ItemCatalog *catalog, GameKind game) {
    partySetStat(record, PartyStatEquipRating1, partyGetU16(record, PartyFieldEquipRatingBase1));
    partySetStat(record, PartyStatEquipRating2, partyGetU16(record, PartyFieldEquipRatingBase2));
    partySetStat(record, PartyStatEquipRating3, partyGetU16(record, PartyFieldEquipRatingBase3));
    partySetStat(record, PartyStatEquipRating4, partyGetU16(record, PartyFieldStrengthBonus));
    partySetStat(record, PartyStatEquipRating5, partyGetU16(record, PartyFieldDexterityBonus));
    partySetStatMax(record, PartyStatEquipRating1, partyGetU16(record, PartyFieldEquipRatingBase1Max));
    partySetStatMax(record, PartyStatEquipRating2, partyGetU16(record, PartyFieldEquipRatingBase2Max));
    partySetStatMax(record, PartyStatEquipRating3, partyGetU16(record, PartyFieldEquipRatingBase3Max));
    partySetStatMax(record, PartyStatEquipRating4, partyGetU16(record, PartyFieldStrengthBonusMax));
    partySetStatMax(record, PartyStatEquipRating5, partyGetU16(record, PartyFieldDexterityBonusMax));

    const uint8_t *weapon = equipTargetEntry(catalog, record, 0x0A, game);
    if (weapon) {
        equipRatingAdd(record, PartyStatEquipRating1, partyGetStat(record, PartyStatProjectile),
                       partyGetStatMax(record, PartyStatProjectile));
        uint16_t bonus = itemTargetWord(weapon, ItemTargetAbsorption);
        equipRatingAdd(record, PartyStatEquipRating2, bonus, bonus);
    }

    /* Unconditional, matching the original -- cleared here, possibly re-set below. */
    partySetU16(record, PartyFieldUiFlags, (uint16_t)(partyGetU16(record, PartyFieldUiFlags) & 0xFFDF));

    const uint8_t *offHand = equipTargetEntry(catalog, record, 0x0C, game);
    if (offHand) {
        uint16_t slotFlags = itemTargetWord(offHand, ItemTargetSlotFlags);
        PartyStat meleeSkill;
        bool hasSkill = true;
        if (slotFlags & 0x4000) {
            meleeSkill = PartyStatSlashing;
        } else if (slotFlags & 0x2000) {
            meleeSkill = PartyStatBashing;
        } else if (slotFlags & 0x1000) {
            meleeSkill = PartyStatPolearm;
        } else {
            hasSkill = false;
            meleeSkill = PartyStatSlashing; /* unused; silences an uninitialized-use warning */
        }
        if (hasSkill) {
            equipRatingAdd(record, PartyStatEquipRating3, partyGetStat(record, meleeSkill),
                           partyGetStatMax(record, meleeSkill));
        }
        uint16_t bonus = itemTargetWord(offHand, ItemTargetAbsorption);
        equipRatingAdd(record, PartyStatEquipRating4, bonus, bonus);

        if (slotFlags & 1) {
            partySetU16(record, PartyFieldUiFlags, (uint16_t)(partyGetU16(record, PartyFieldUiFlags) | 0x20));
        }
    }

    for (unsigned code = 0x0D; code <= 0x14; code++) {
        const uint8_t *entry = equipTargetEntry(catalog, record, code, game);
        if (entry) {
            uint16_t bonus = itemTargetWord(entry, ItemTargetAbsorption);
            equipRatingAdd(record, PartyStatEquipRating5, bonus, bonus);
        }
    }
}

/*
 * DS:0xD22B (yendor2.asm:21785), extracted via
 * ida_scripts/dump_ability_unlock_table.py. Row order matches class
 * base 4 (MONK) through base 9 (MARKSMAN). Each row is 20 columns
 * (even levels 2..40) x 2 slots; a 0 slot means "no ability here" and,
 * per the original, ends that column's scan (a nonzero slot after a
 * zero one is never real).
 */
static const PartyAbilityUnlockRow g_abilityUnlocksYendor2[PartyAbilityUnlockRowCount] = {
    {0x0005, 0x0000, 0x0007, 0x000b, 0x000e, 0x0010, 0x0016, 0x0017, 0x001b, 0x001d, 0x0022, 0x0023, 0x0026, 0x0027,
     0x002d, 0x002e, 0x0034, 0x0037, 0x003d, 0x003f, 0x0044, 0x0045, 0x0049, 0x004b, 0x004d, 0x004f, 0x0051, 0x0052,
     0x0055, 0x0000, 0x0059, 0x005a, 0x0060, 0x0061, 0x0063, 0x0065, 0x0068, 0x0000, 0x0000, 0x0000},
    {0x0005, 0x0000, 0x0007, 0x000b, 0x000a, 0x000e, 0x0010, 0x0015, 0x000f, 0x0017, 0x0018, 0x0022, 0x001d, 0x0027,
     0x0021, 0x002e, 0x0035, 0x0037, 0x002f, 0x003d, 0x0044, 0x0034, 0x0049, 0x004a, 0x003f, 0x004f, 0x0051, 0x0052,
     0x0054, 0x0000, 0x005c, 0x0000, 0x005f, 0x0000, 0x005d, 0x0000, 0x0065, 0x0000, 0x0000, 0x0000},
    {0x0003, 0x0000, 0x0002, 0x0006, 0x0005, 0x000b, 0x000e, 0x0016, 0x0010, 0x0014, 0x0017, 0x001b, 0x001e, 0x0024,
     0x0019, 0x0030, 0x0026, 0x002a, 0x0031, 0x0000, 0x0037, 0x0000, 0x003f, 0x0000, 0x0045, 0x0000, 0x0052, 0x0000,
     0x004d, 0x0000, 0x0049, 0x0000, 0x0051, 0x0000, 0x0055, 0x0000, 0x005a, 0x0000, 0x0000, 0x0000},
    {0x0004, 0x0000, 0x0009, 0x000a, 0x000d, 0x000f, 0x0016, 0x0018, 0x001c, 0x001d, 0x0021, 0x0023, 0x0028, 0x0029,
     0x002c, 0x002f, 0x0033, 0x0036, 0x003c, 0x003e, 0x0046, 0x0047, 0x0048, 0x0000, 0x004c, 0x004e, 0x0053, 0x0000,
     0x0056, 0x0000, 0x0057, 0x005b, 0x005d, 0x005e, 0x0062, 0x0066, 0x0067, 0x0069, 0x0000, 0x0000},
    {0x0004, 0x0000, 0x0007, 0x000a, 0x000c, 0x000f, 0x000e, 0x0015, 0x0018, 0x001c, 0x0017, 0x001d, 0x0021, 0x0029,
     0x0019, 0x002f, 0x0035, 0x0036, 0x002a, 0x003c, 0x0033, 0x0047, 0x0048, 0x004a, 0x0046, 0x004c, 0x0053, 0x0000,
     0x0054, 0x0000, 0x0058, 0x0049, 0x0056, 0x005f, 0x005d, 0x0064, 0x0069, 0x0000, 0x0000, 0x0000},
    {0x0003, 0x0000, 0x0004, 0x0006, 0x000a, 0x0000, 0x0009, 0x0016, 0x000d, 0x001c, 0x0023, 0x0000, 0x0018, 0x0021,
     0x002f, 0x0000, 0x002b, 0x0000, 0x0040, 0x0000, 0x0036, 0x0000, 0x0033, 0x0041, 0x003c, 0x0000, 0x0046, 0x0000,
     0x004c, 0x0000, 0x0048, 0x0000, 0x0053, 0x0000, 0x004e, 0x0000, 0x0056, 0x0000, 0x0000, 0x0000},
};

/* DS:0xB8B5 (yendor3.asm:21785-ish), extracted the same way. Same shape, different ids -- a real per-game content difference. */
static const PartyAbilityUnlockRow g_abilityUnlocksYendor3[PartyAbilityUnlockRowCount] = {
    {0x0005, 0x0000, 0x0006, 0x0007, 0x000e, 0x0010, 0x0016, 0x0017, 0x001b, 0x001d, 0x0022, 0x0023, 0x0026, 0x0027,
     0x002d, 0x002e, 0x0034, 0x0037, 0x003d, 0x003f, 0x0044, 0x0045, 0x0049, 0x004d, 0x0050, 0x004f, 0x0052, 0x0000,
     0x0055, 0x0000, 0x005a, 0x0000, 0x0062, 0x0063, 0x0065, 0x0067, 0x006a, 0x0000, 0x0000, 0x0000},
    {0x0005, 0x0000, 0x0006, 0x0007, 0x000a, 0x000e, 0x0010, 0x0015, 0x000f, 0x0017, 0x0018, 0x0022, 0x001d, 0x0027,
     0x0021, 0x002e, 0x0035, 0x0037, 0x002f, 0x003d, 0x0044, 0x0034, 0x0049, 0x004d, 0x003f, 0x004f, 0x0052, 0x0000,
     0x0054, 0x0000, 0x005c, 0x0000, 0x0061, 0x0000, 0x005f, 0x0000, 0x0067, 0x0000, 0x0000, 0x0000},
    {0x0003, 0x0000, 0x0002, 0x0006, 0x0005, 0x0007, 0x000e, 0x0016, 0x0010, 0x0011, 0x0017, 0x001b, 0x002d, 0x0024,
     0x0019, 0x0030, 0x0026, 0x002a, 0x0031, 0x0000, 0x0037, 0x0000, 0x003f, 0x004d, 0x0045, 0x004f, 0x0052, 0x0000,
     0x0050, 0x0000, 0x0049, 0x0000, 0x0055, 0x0000, 0x0053, 0x0000, 0x005a, 0x0000, 0x0000, 0x0000},
    {0x0004, 0x0000, 0x0009, 0x000a, 0x000d, 0x000f, 0x0016, 0x0018, 0x001c, 0x001d, 0x0021, 0x0023, 0x0028, 0x0029,
     0x002c, 0x002f, 0x0033, 0x0036, 0x003e, 0x0000, 0x0046, 0x0047, 0x004b, 0x0000, 0x004c, 0x004e, 0x0053, 0x0000,
     0x0056, 0x0000, 0x0059, 0x005b, 0x005f, 0x0060, 0x0064, 0x0068, 0x0069, 0x006b, 0x0000, 0x0000},
    {0x0004, 0x0000, 0x0006, 0x000a, 0x000c, 0x000f, 0x000e, 0x0015, 0x0018, 0x001c, 0x0017, 0x001d, 0x0021, 0x0029,
     0x0019, 0x002f, 0x0035, 0x0036, 0x002a, 0x0000, 0x0033, 0x0047, 0x004a, 0x004b, 0x004c, 0x004f, 0x0053, 0x0000,
     0x0054, 0x0000, 0x0049, 0x0000, 0x0056, 0x0061, 0x005f, 0x0066, 0x006b, 0x0000, 0x0000, 0x0000},
    {0x0003, 0x0000, 0x0004, 0x0006, 0x000a, 0x0000, 0x0009, 0x0016, 0x000d, 0x001c, 0x0023, 0x0000, 0x0018, 0x0021,
     0x002f, 0x0000, 0x002b, 0x0000, 0x0040, 0x0000, 0x0036, 0x0000, 0x0033, 0x0041, 0x003e, 0x0000, 0x0046, 0x0000,
     0x004c, 0x0000, 0x004b, 0x0000, 0x0053, 0x0000, 0x004e, 0x0000, 0x0056, 0x0000, 0x0000, 0x0000},
};

const PartyAbilityUnlockRow *partyAbilityUnlockTable(GameKind game) {
    return game == GameYendor2 ? g_abilityUnlocksYendor2 : g_abilityUnlocksYendor3;
}

unsigned partyAbilityUnlocksAtLevel(unsigned classId, unsigned level, GameKind game, uint16_t out[PartyAbilityUnlockSlotCount]) {
    if (level % 2 != 0 || level < 2 || level > PartyAbilityUnlockColumnCount * 2) {
        return 0;
    }
    unsigned base = partyClassBase(classId);
    if (base < 4 || base > 9) {
        return 0;
    }
    unsigned row = base - 4;
    unsigned col = level / 2 - 1;
    const uint16_t *slots = partyAbilityUnlockTable(game)[row] + col * PartyAbilityUnlockSlotCount;

    unsigned count = 0;
    if (slots[0] == 0) {
        return 0;
    }
    out[count++] = slots[0];
    if (slots[1] == 0) {
        return count;
    }
    out[count++] = slots[1];
    return count;
}

unsigned partyApplyAbilityUnlocks(uint8_t *record, unsigned classId, unsigned level, GameKind game) {
    uint16_t ids[PartyAbilityUnlockSlotCount];
    unsigned count = partyAbilityUnlocksAtLevel(classId, level, game, ids);
    for (unsigned i = 0; i < count; i++) {
        flagBankSet(record + PartyFieldFlagBankCA, 16, ids[i]);
    }
    return count;
}

bool partyClassIsValid(unsigned classId) {
    unsigned base = classId % 10;
    return classId >= 1 && classId <= 29 && base != 0;
}

unsigned partyClassBase(unsigned classId) {
    return classId % 10;
}

unsigned partyClassTier(unsigned classId) {
    return classId / 10;
}

const char *partyClassName(unsigned classId) {
    if (!partyClassIsValid(classId)) {
        return NULL;
    }
    return g_classNames[classId / 10][classId % 10 - 1];
}

uint16_t partyClassSecondaryBit(unsigned classId) {
    if (classId < 4 || classId > 9) {
        return 0;
    }
    return (uint16_t)(0x20u >> (classId - 4));
}

static bool flagLocate(unsigned words, unsigned index, unsigned *wordIndex, uint16_t *mask) {
    if (index == 0 || index > words * 16) {
        return false;
    }
    *wordIndex = (index - 1) / 16;
    *mask = (uint16_t)(0x8000u >> ((index - 1) % 16));
    return true;
}

bool flagBankTest(const uint8_t *bank, unsigned words, unsigned index) {
    unsigned wordIndex;
    uint16_t mask;
    if (!flagLocate(words, index, &wordIndex, &mask)) {
        return false;
    }
    return (partyGetU16(bank, wordIndex * 2) & mask) != 0;
}

void flagBankSet(uint8_t *bank, unsigned words, unsigned index) {
    unsigned wordIndex;
    uint16_t mask;
    if (flagLocate(words, index, &wordIndex, &mask)) {
        partySetU16(bank, wordIndex * 2, (uint16_t)(partyGetU16(bank, wordIndex * 2) | mask));
    }
}

void flagBankClear(uint8_t *bank, unsigned words, unsigned index) {
    unsigned wordIndex;
    uint16_t mask;
    if (flagLocate(words, index, &wordIndex, &mask)) {
        partySetU16(bank, wordIndex * 2, (uint16_t)(partyGetU16(bank, wordIndex * 2) & ~mask));
    }
}

enum {
    AbilityFlagWords = 16,
    EventFlagWords = 6
};

bool partyTestAbilityFlag(const uint8_t *record, unsigned index) {
    return flagBankTest(record + PartyFieldFlagBankCA, AbilityFlagWords, index);
}

void partySetAbilityFlag(uint8_t *record, unsigned index) {
    flagBankSet(record + PartyFieldFlagBankCA, AbilityFlagWords, index);
}

bool partyTestEventFlag(const uint8_t *record, unsigned index) {
    return flagBankTest(record + PartyFieldFlagBank10C, EventFlagWords, index);
}

void partySetEventFlag(uint8_t *record, unsigned index) {
    flagBankSet(record + PartyFieldFlagBank10C, EventFlagWords, index);
}

uint8_t *partyBagMarker(uint8_t *record, unsigned bag) {
    if (bag >= 3) {
        return NULL;
    }
    return record + PartyFieldBagMarkers + bag * PartyBagMarkerSize;
}

uint8_t *partyInventoryGroup(uint8_t *record, PartyGroup group) {
    if (group == PartyGroupMain) {
        return record + PartyFieldInventory;
    }
    if ((unsigned)group >= PartyGroupCount) {
        return NULL;
    }
    return partyBagMarker(record, group - PartyGroupBag1) + 4;
}

uint8_t *partyActiveInventoryGroup(uint8_t *record) {
    for (unsigned bag = 0; bag < 3; bag++) {
        if (partyGetU16(partyBagMarker(record, bag), 0) != 0) {
            return partyBagMarker(record, bag) + 4;
        }
    }
    return record + PartyFieldInventory;
}

uint16_t inventoryGroupWeight(const uint8_t *group) {
    return partyGetU16(group, 0);
}

void inventoryGroupSetWeight(uint8_t *group, uint16_t weight) {
    partySetU16(group, 0, weight);
}

uint8_t *inventoryGroupSlot(uint8_t *group, unsigned slot) {
    if (slot < 1 || slot > InventorySlotCount) {
        return NULL;
    }
    return group + 2 + (slot - 1) * ItemSlotSize;
}

uint16_t itemSlotId(const uint8_t *slot) {
    return partyGetU16(slot, 0);
}

uint16_t itemSlotExtra(const uint8_t *slot) {
    return partyGetU16(slot, 2);
}

void itemSlotSet(uint8_t *slot, uint16_t id, uint16_t extra) {
    partySetU16(slot, 0, id);
    partySetU16(slot, 2, extra);
}

uint8_t *partyEquipmentSlot(uint8_t *record, unsigned code, GameKind game) {
    if (code < PartyEquipmentFirst || code > PartyEquipmentLast) {
        return NULL;
    }
    if (game == GameYendor3 && code == 0x12) {
        code = 0x14;
    }
    if (code < PartyEquipmentFirstShort) {
        return record + PartyFieldEquipment + (code - PartyEquipmentFirst) * ItemSlotSize;
    }
    return record + 0x152 + (code - PartyEquipmentFirstShort) * 2;
}

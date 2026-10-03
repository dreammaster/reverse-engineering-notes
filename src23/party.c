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

void partyDeductHp(uint8_t *record, uint16_t amount) {
    int16_t hp = (int16_t)(partyGetStat(record, PartyStatHitPoints) - amount);
    if (hp <= 0) {
        partySetStat(record, PartyStatHitPoints, 0);
        partySetU16(record, PartyFieldStatusFlags, (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) | PartyStatusDead));
        return;
    }
    partySetStat(record, PartyStatHitPoints, (uint16_t)hp);
}

void partyDeductMp(uint8_t *record, uint16_t amount) {
    int16_t mp = (int16_t)(partyGetStat(record, PartyStatMagicPoints) - amount);
    partySetStat(record, PartyStatMagicPoints, mp > 0 ? (uint16_t)mp : 0);
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

void partyApplyIconBarStatDelta(uint8_t *record, GameKind game, uint16_t modeFlags, uint16_t delta,
                                 unsigned currentFieldOffset, unsigned maxFieldOffset,
                                 uint16_t statusFlagsClearMask) {
    enum { EffectModeStatFloorBit = 0x0080, EffectModeStatCappedBit = 0x0100 };

    if (modeFlags & EffectModeStatCappedBit) {
        uint16_t value = (uint16_t)(partyGetU16(record, currentFieldOffset) + delta);
        if (maxFieldOffset != 0) {
            uint16_t max = partyGetU16(record, maxFieldOffset);
            if (value > max) {
                value = max;
            }
        }
        partySetU16(record, currentFieldOffset, value);
    } else if (modeFlags & EffectModeStatFloorBit) {
        int32_t value = (int32_t)partyGetU16(record, currentFieldOffset) - (int32_t)delta;
        if (value < 0) {
            value = 0;
        }
        partySetU16(record, currentFieldOffset, (uint16_t)value);
    }

    partySetU16(record, PartyFieldStatusFlags,
                (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) & statusFlagsClearMask));
    partyRefreshCarryCapacityAndAttributeBonuses(record);
    partyCheckForLevelUp(record, game);
}

enum { PartyMultiStatEffectSplit = 0x32 };

void partyApplyMultiStatEffect(uint8_t *record, const uint8_t *effect) {
    if (!effect) {
        return;
    }
    unsigned pairs = itemEffectPairs(effect);
    for (unsigned i = 0; i < pairs; i++) {
        unsigned field = itemEffectField(effect, i);
        uint16_t amount = itemEffectAmount(effect, i);
        if (field >= PartyMultiStatEffectSplit && partyGetU16(record, field) == 0) {
            continue;
        }
        uint16_t value = (uint16_t)(partyGetU16(record, field) + amount);
        if (value > 999) {
            value = 999;
        }
        partySetU16(record, field, value);
    }
}

void partyRemoveMultiStatEffect(uint8_t *record, const uint8_t *effect) {
    if (!effect) {
        return;
    }
    unsigned pairs = itemEffectPairs(effect);
    for (unsigned i = 0; i < pairs; i++) {
        unsigned field = itemEffectField(effect, i);
        uint16_t amount = itemEffectAmount(effect, i);
        if (field >= PartyMultiStatEffectSplit && partyGetU16(record, field) == 0) {
            continue;
        }
        uint16_t value = (uint16_t)(partyGetU16(record, field) - amount);
        if (field >= PartyMultiStatEffectSplit && (int16_t)value < 0) {
            value = 0;
        }
        partySetU16(record, field, value);
    }
}

void partyHandleIconBarItemExpiry(uint8_t *record, const ItemCatalog *catalog, uint16_t modeFlags,
                                   uint16_t equippedItemId, uint16_t replacementItemId, unsigned slotOffset) {
    enum { EffectModeItemDestroyBit = 0x0200 };

    const uint8_t *equippedRecord = itemCatalogRecord(catalog, equippedItemId);
    partyRemoveMultiStatEffect(record, equippedRecord ? itemEffectEntry(catalog, equippedRecord) : NULL);

    uint8_t *slot = record + slotOffset;
    if (modeFlags & EffectModeItemDestroyBit) {
        partySetU16(slot, 0, 0);
        if (equippedRecord) {
            uint16_t weight = itemGetU16(equippedRecord, ItemFieldWeight);
            partySetU16(record, PartyFieldInventory, (uint16_t)(partyGetU16(record, PartyFieldInventory) - weight));
        }
    } else {
        itemSlotSet(slot, replacementItemId, equippedItemId);
        const uint8_t *replacementRecord = itemCatalogRecord(catalog, replacementItemId);
        partyApplyMultiStatEffect(record, replacementRecord ? itemEffectEntry(catalog, replacementRecord) : NULL);
    }

    partyRefreshCarryCapacityAndAttributeBonuses(record);
}

PartyItemDurabilityOutcome partyTickEquippedItemDurability(uint8_t *record, const ItemCatalog *catalog,
                                                             GameKind game, unsigned slotOffset, RandomState *rng) {
    uint16_t itemId = partyGetU16(record, slotOffset);
    if (itemId == 0) {
        return PartyItemDurabilityUnchanged;
    }

    const uint8_t *itemRecord = itemCatalogRecord(catalog, itemId);
    ItemServiceTier tier;
    if (!itemRecord || !itemClassifyServiceTier(itemRecord, &tier)) {
        return PartyItemDurabilityUnchanged;
    }

    unsigned wearFieldOffset;
    uint16_t threshold;
    if (slotOffset == 0x13A) {
        wearFieldOffset = PartyFieldWearMain;
        threshold = 0x78;
    } else if (slotOffset == 0x142) {
        wearFieldOffset = PartyFieldWearSecond;
        threshold = 0x50;
    } else {
        wearFieldOffset = PartyFieldWearThird;
        threshold = 0x14;
    }

    uint16_t wear = (uint16_t)(partyGetU16(record, wearFieldOffset) + 1);
    partySetU16(record, wearFieldOffset, wear);
    if (wear <= threshold) {
        return PartyItemDurabilityUnchanged;
    }

    const uint8_t *targetEntry = itemTargetEntry(catalog, itemRecord);
    if (!targetEntry) {
        return PartyItemDurabilityUnchanged;
    }
    bool categoryA = (itemGetU16(itemRecord, ItemFieldFlags) & (ItemFlagEquipCode0A | ItemFlagEquipCode0C)) != 0;
    uint16_t chance = itemTargetWord(targetEntry, categoryA ? ItemTargetBreakChanceA : ItemTargetBreakChanceB);
    uint16_t roll = randomInRange(rng, 1000);
    if (roll > chance) {
        return PartyItemDurabilityUnchanged;
    }

    uint16_t replacementId = itemTargetWord(targetEntry, categoryA ? ItemTargetBreakItemA : ItemTargetBreakItemB);

    EffectDef def;
    uint16_t modeFlags = effectGetDef(game, 0, &def) ? def.modeFlags : 0;
    partyHandleIconBarItemExpiry(record, catalog, modeFlags, itemId, replacementId, slotOffset);

    unsigned resetOffset = PartyFieldWearMain;
    const uint8_t *replacementRecord = itemCatalogRecord(catalog, replacementId);
    if (replacementRecord) {
        uint16_t replacementFlags = itemGetU16(replacementRecord, ItemFieldFlags);
        if (replacementFlags & ItemFlagEquipCode0A) {
            resetOffset = PartyFieldWearMain;
        } else if (replacementFlags & ItemFlagEquipCode0C) {
            resetOffset = PartyFieldWearSecond;
        } else {
            resetOffset = PartyFieldWearThird;
        }
    }
    partySetU16(record, resetOffset, 0);
    return PartyItemDurabilityBroke;
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

unsigned partyKnownAbilityIdMax(GameKind game) {
    return game == GameYendor2 ? 125 : 107;
}

unsigned partyKnownAbilityIds(const uint8_t *record, GameKind game, unsigned *out, unsigned outCapacity) {
    unsigned found = 0;
    unsigned max = partyKnownAbilityIdMax(game);
    for (unsigned id = 1; id <= max; id++) {
        if (flagBankTest(record + PartyFieldFlagBankCA, 16, id)) {
            if (found < outCapacity) {
                out[found] = id;
            }
            found++;
        }
    }
    return found;
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

bool partyDecodeSavingThrowEffect(uint16_t packedValue, PartySavingThrowEffect *out) {
    if (packedValue == 0) {
        return false;
    }
    out->threshold = packedValue / 100;
    unsigned effectId = packedValue % 100;
    out->wholeParty = effectId >= 50;
    out->effectId = out->wholeParty ? effectId - 50 : effectId;
    return true;
}

void partyResetDailyAbilityCharges(uint8_t *record) {
    for (unsigned i = 0; i < 4; i++) {
        partySetU16(record, PartyFieldAbilityCharge + i * 2, 0);
    }
}

PartyRestOutcome partyApplyRestEffects(uint8_t *record, uint16_t regenPercent) {
    PartyRestOutcome outcome = {false, false};
    uint16_t status = partyGetU16(record, PartyFieldStatusFlags);
    if (status & PartyStatusIncapacitated) {
        outcome.wasSkipped = true;
        return outcome;
    }

    enum {
        PartyRestAbnormalMask = PartyStatusSick | PartyStatusPoisoned | PartyStatusDiseased | PartyStatusHexed |
                                 PartyStatusJinxed | PartyStatusCursed
    };
    if (status & PartyRestAbnormalMask) {
        if (status & PartyStatusSick) {
            status = (uint16_t)(status & ~(uint16_t)PartyStatusSick);
        }
        if (status & PartyStatusJinxed) {
            status = (uint16_t)(status & ~(uint16_t)PartyStatusJinxed);
        }
        partySetU16(record, PartyFieldStatusFlags, status);

        if (status & PartyStatusDiseased) {
            partyDeductHp(record, 36);
            if (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead) {
                outcome.died = true;
                return outcome;
            }
        }
        if (status & PartyStatusCursed) {
            partyDeductMp(record, 48);
        }
        return outcome;
    }

    uint16_t maxHp = partyGetStatMax(record, PartyStatHitPoints);
    uint32_t hp = ((uint32_t)maxHp * regenPercent + 50) / 100 + partyGetStat(record, PartyStatHitPoints);
    partySetStat(record, PartyStatHitPoints, hp > maxHp ? maxHp : (uint16_t)hp);

    uint16_t maxMp = partyGetStatMax(record, PartyStatMagicPoints);
    if (maxMp != 0) {
        uint32_t mp = ((uint32_t)maxMp * regenPercent + 50) / 100 + partyGetStat(record, PartyStatMagicPoints);
        partySetStat(record, PartyStatMagicPoints, mp > maxMp ? maxMp : (uint16_t)mp);
    }
    return outcome;
}

uint16_t partyFindItemInRange(const uint8_t *record, uint16_t lowId, uint16_t highId, unsigned *outSlotOffset) {
    for (unsigned slot = 1; slot <= 8; slot++) {
        unsigned offset = PartyFieldInventory + 2 + (slot - 1) * ItemSlotSize;
        uint16_t id = itemSlotId(record + offset);
        if (id == 0) {
            continue;
        }
        if (id >= lowId && id <= highId) {
            if (outSlotOffset) {
                *outSlotOffset = offset;
            }
            return id;
        }
    }
    return 0;
}

enum { ContainerSearchMaxDepth = 3, EquipmentContainerSlotOffset = 0x13E };

static bool itemIsContainer(const ItemCatalog *catalog, uint16_t id) {
    const uint8_t *item = itemCatalogRecord(catalog, id);
    return item && (itemGetU16(item, ItemFieldFlags) & ItemFlagEquipCode0B);
}

/* One FindItemInsideContainer level: instance record `number`, at nesting `level` (1-3). */
static bool searchContainer(SaveGame *save, const ItemCatalog *catalog, unsigned number, unsigned level, uint16_t lowId,
                            uint16_t highId, PartyDeepFind *out) {
    uint8_t *contents = saveGameRecord(save, SaveSectionItemInstances, number);
    if (!contents) {
        return false;
    }
    out->containerRecords[level - 1] = number;
    for (unsigned slot = 1; slot <= 8; slot++) {
        const uint8_t *entry = inventoryGroupSlot(contents, slot);
        uint16_t id = itemSlotId(entry);
        if (id == 0) {
            continue;
        }
        if (id >= lowId && id <= highId) {
            out->itemId = id;
            out->depth = level;
            out->slotOffset = (unsigned)(entry - contents);
            return true;
        }
        if (level < ContainerSearchMaxDepth && itemIsContainer(catalog, id) &&
            searchContainer(save, catalog, itemSlotExtra(entry), level + 1, lowId, highId, out)) {
            return true;
        }
    }
    return false;
}

PartyDeepFind partyFindItemDeep(const uint8_t *partyRecord, SaveGame *save, const ItemCatalog *catalog, uint16_t lowId,
                                uint16_t highId) {
    PartyDeepFind out;
    memset(&out, 0, sizeof(out));
    for (unsigned slot = 1; slot <= 8; slot++) {
        unsigned offset = PartyFieldInventory + 2 + (slot - 1) * ItemSlotSize;
        const uint8_t *entry = partyRecord + offset;
        uint16_t id = itemSlotId(entry);
        if (id == 0) {
            continue;
        }
        if (id >= lowId && id <= highId) {
            out.itemId = id;
            out.slotOffset = offset;
            return out;
        }
        if (itemIsContainer(catalog, id) && searchContainer(save, catalog, itemSlotExtra(entry), 1, lowId, highId, &out)) {
            return out;
        }
    }
    const uint8_t *equipped = partyRecord + EquipmentContainerSlotOffset;
    uint16_t id = itemSlotId(equipped);
    if (id != 0 && itemIsContainer(catalog, id) &&
        searchContainer(save, catalog, itemSlotExtra(equipped), 1, lowId, highId, &out)) {
        return out;
    }
    memset(&out, 0, sizeof(out));
    return out;
}

ItemRangeAvailability itemRangeAvailable(const uint8_t *globalSlots, SaveGame *save, uint16_t lowId, uint16_t highId) {
    ItemRangeAvailability result;
    memset(&result, 0, sizeof(result));
    if (lowId == 0 || lowId > highId) {
        return result;
    }

    for (unsigned slot = 0; slot < 6; slot++) {
        const uint8_t *entry = globalSlots + slot * 4;
        uint16_t id = itemSlotId(entry);
        if (id >= lowId && id <= highId) {
            result.found = true;
            result.itemId = id;
            result.inGlobalTable = true;
            result.slotOffset = slot * 4;
            return result;
        }
    }

    for (unsigned member = 0; member < SavePartyMemberSlots; member++) {
        uint16_t id = saveGetPartySlot(save, member);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record) {
            continue;
        }
        unsigned offset;
        uint16_t found = partyFindItemInRange(record, lowId, highId, &offset);
        if (found != 0) {
            result.found = true;
            result.itemId = found;
            result.inGlobalTable = false;
            result.slotOffset = offset;
            result.partyRecordId = id;
            return result;
        }
    }
    return result;
}

/*
 * The "decrement-or-discard" step both of ConsumeItemChargeResource's
 * default-mode branches share. Returns the item's own id (now cleared
 * from the slot) if it was just discarded, or 0 if a charge was merely
 * spent and the item remains.
 */
static uint16_t itemSlotSpendCharge(const ItemCatalog *catalog, uint8_t *slot) {
    uint16_t id = itemSlotId(slot);
    const uint8_t *record = itemCatalogRecord(catalog, id);
    const uint8_t *entry = record ? itemTargetEntry(catalog, record) : NULL;

    if (entry && (itemTargetWord(entry, 1) & 1)) {
        uint16_t extra = itemSlotExtra(slot);
        if (extra > 0) {
            extra--;
        }
        if (extra > 0) {
            itemSlotSet(slot, id, extra);
            return 0;
        }
    }

    itemSlotSet(slot, 0, 0);
    return id;
}

void partyConsumeItemCharge(uint8_t *partyRecord, const ItemCatalog *catalog, uint8_t *slot) {
    uint16_t discardedId = itemSlotSpendCharge(catalog, slot);
    if (discardedId == 0) {
        return;
    }
    const uint8_t *record = itemCatalogRecord(catalog, discardedId);
    uint8_t *mainGroup = partyInventoryGroup(partyRecord, PartyGroupMain);
    uint16_t weight = record ? itemGetU16(record, ItemFieldWeight) : 0;
    inventoryGroupSetWeight(mainGroup, (uint16_t)(inventoryGroupWeight(mainGroup) - weight));
}

static void discardPartySlot(uint8_t *partyRecord, const ItemCatalog *catalog, GameKind game, unsigned slotOffset) {
    uint8_t *slot = partyRecord + slotOffset;
    uint16_t id = itemSlotId(slot);
    partySetU16(slot, 0, 0);
    if (game != GameYendor3 || slotOffset <= 0x14E) {
        partySetU16(slot, 2, 0);
    }
    const uint8_t *record = itemCatalogRecord(catalog, id);
    if (game == GameYendor3 && slotOffset >= 0x142 && slotOffset <= 0x15A) {
        partyRemoveMultiStatEffect(partyRecord, record ? itemEffectEntry(catalog, record) : NULL);
    }
    uint8_t *mainGroup = partyInventoryGroup(partyRecord, PartyGroupMain);
    uint16_t weight = record ? itemGetU16(record, ItemFieldWeight) : 0;
    inventoryGroupSetWeight(mainGroup, (uint16_t)(inventoryGroupWeight(mainGroup) - weight));
}

void partyConsumeItemChargeMode(uint8_t *partyRecord, const ItemCatalog *catalog, GameKind game, unsigned slotOffset,
                                uint16_t currentItemId, ItemChargeMode mode) {
    uint8_t *slot = partyRecord + slotOffset;
    switch (mode) {
    case ItemChargeRecharge: {
        itemSlotSet(slot, itemSlotExtra(slot), 0);
        if (slotOffset == 0x142) {
            partySetU16(partyRecord, PartyFieldWearSecond, 0);
        } else if (slotOffset == 0x13A) {
            partySetU16(partyRecord, PartyFieldWearMain, 0);
        } else if (slotOffset == 0x146) {
            partySetU16(partyRecord, PartyFieldWearThird, 0);
        }
        break;
    }
    case ItemChargeDiscard:
        discardPartySlot(partyRecord, catalog, game, slotOffset);
        break;
    case ItemChargeSwap: {
        const uint8_t *current = itemCatalogRecord(catalog, currentItemId);
        const uint8_t *entry = current ? itemTargetEntry(catalog, current) : NULL;
        if (!current || !entry) {
            break;
        }
        uint16_t flags = itemGetU16(current, ItemFieldFlags);
        unsigned replacementWord;
        if (flags & 0xC000) {
            if (!(itemTargetWord(entry, 1) & 0x200)) {
                break;
            }
            replacementWord = 2;
        } else if (flags & 0x800) {
            if (!(itemTargetWord(entry, 1) & 0x80)) {
                break;
            }
            replacementWord = 4;
        } else {
            break;
        }
        partyRemoveMultiStatEffect(partyRecord, itemEffectEntry(catalog, current));
        uint16_t replacementId = itemTargetWord(entry, replacementWord);
        itemSlotSet(slot, replacementId, currentItemId);
        const uint8_t *replacement = itemCatalogRecord(catalog, replacementId);
        partyApplyMultiStatEffect(partyRecord, replacement ? itemEffectEntry(catalog, replacement) : NULL);
        break;
    }
    default: {
        uint16_t id = itemSlotId(slot);
        const uint8_t *record = itemCatalogRecord(catalog, id);
        const uint8_t *entry = record ? itemTargetEntry(catalog, record) : NULL;
        if (entry && (itemTargetWord(entry, 1) & 1)) {
            uint16_t extra = itemSlotExtra(slot);
            if (extra > 0) {
                extra--;
            }
            if (extra > 0) {
                itemSlotSet(slot, id, extra);
                break;
            }
        }
        discardPartySlot(partyRecord, catalog, game, slotOffset);
        break;
    }
    }
}

void itemSlotConsumeGlobalCharge(const ItemCatalog *catalog, uint8_t *slot) {
    itemSlotSpendCharge(catalog, slot);
}

uint16_t partyDeriveRestRegenPercent(uint8_t *globalSlots, SaveGame *save, const ItemCatalog *catalog) {
    unsigned activeCount = 0;
    for (unsigned member = 0; member < SavePartyMemberSlots; member++) {
        uint16_t id = saveGetPartySlot(save, member);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (record && !(partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
            activeCount++;
        }
    }
    if (activeCount == 0) {
        return 0;
    }

    unsigned consumed = 0;
    for (unsigned i = 0; i < activeCount; i++) {
        ItemRangeAvailability avail = itemRangeAvailable(globalSlots, save, 0x36, 0x40);
        if (!avail.found) {
            break;
        }
        if (avail.inGlobalTable) {
            itemSlotConsumeGlobalCharge(catalog, globalSlots + avail.slotOffset);
        } else {
            uint8_t *record = saveGamePartyRecordById(save, avail.partyRecordId);
            partyConsumeItemCharge(record, catalog, record + avail.slotOffset);
        }
        consumed++;
    }
    return (uint16_t)((100 / activeCount) * consumed);
}

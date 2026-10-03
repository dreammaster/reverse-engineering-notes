#include "chargen.h"

#include <string.h>

static const DerivedStatRule g_derivedRulesYendor2[] = {
    {0x58, 3, {{PartyStatStrength, 10}, {PartyStatDexterity, 30}, {PartyStatStamina, 60}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 4, 3, 0, 0, 1, 0, 0, 2}}, /* Survival */
    {0x5A, 2, {{PartyStatStrength, 80}, {PartyStatDexterity, 20}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 5, -5, -5, 0, -5, -5, 0}}, /* Projectile */
    {0x5C, 2, {{PartyStatStrength, 20}, {PartyStatDexterity, 80}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Slashing */
    {0x5E, 1, {{PartyStatStrength, 0}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Bashing */
    {0x60, 2, {{PartyStatStrength, 50}, {PartyStatDexterity, 50}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Polearm */
    {0x64, 2, {{PartyStatIntelligence, 90}, {PartyStatWisdom, 10}}, {-1, 40, -1, 40, -1, 40, 40, -1, -1, -1}, {0, 0, 0, 0, 0, 0, 0, 5, 3, 3}}, /* Mapping */
    {0x66, 1, {{PartyStatDexterity, 0}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 0, 3, 3, 0, 0, 5, 0, 0, 5}}, /* Navigation */
    {0x68, 2, {{PartyStatIntelligence, 15}, {PartyStatCharisma, 85}}, {-1, 40, -1, -1, 40, 0, -1, 40, 40, 0}, {0, 0, 5, -2, 0, 0, 0, 0, 0, 0}}, /* Bartering */
    {0x6A, 2, {{PartyStatDexterity, 80}, {PartyStatIntelligence, 20}}, {-1, -1, -1, -1, 40, 40, 0, 0, -1, 40}, {0, 3, 5, -2, 0, 0, 0, 0, 0, 0}}, /* Repair */
    {0x6C, 1, {{PartyStatDexterity, 0}}, {-1, 40, -1, -1, 0, -1, 40, 40, 0, -1}, {0, 0, -7, 5, 0, 0, 0, 0, 0, 0}}, /* Thievery */
    {0x6E, 2, {{PartyStatIntelligence, 70}, {PartyStatWisdom, 30}}, {-1, 0, -1, 0, 40, 40, 40, -1, -1, -1}, {0, 0, -5, 0, 0, 0, 0, 1, 5, -5}}, /* Linguistics */
    {0x70, 2, {{PartyStatIntelligence, 20}, {PartyStatWisdom, 80}}, {-1, 0, 0, 0, -1, -1, -1, 40, 40, 40}, {0, 0, 0, 0, 0, 5, -5, 0, 0, 0}}, /* Chemistry */
};

static const DerivedStatRule g_derivedRulesYendor3[] = {
    {0x58, 3, {{PartyStatStrength, 10}, {PartyStatDexterity, 30}, {PartyStatStamina, 60}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 4, 3, 0, 0, 1, 0, 0, 2}}, /* Survival */
    {0x5A, 2, {{PartyStatStrength, 80}, {PartyStatDexterity, 20}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 5, -5, -5, 0, -5, -5, 0}}, /* Projectile */
    {0x5C, 2, {{PartyStatStrength, 20}, {PartyStatDexterity, 80}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Slashing */
    {0x5E, 1, {{PartyStatStrength, 0}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Bashing */
    {0x60, 2, {{PartyStatStrength, 50}, {PartyStatDexterity, 50}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 5, 0, 0, -7, -7, -5, -7, -7, -5}}, /* Polearm */
    {0x64, 2, {{PartyStatIntelligence, 90}, {PartyStatWisdom, 10}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, -5, 0, 0, 0, -5, -5, 5, 3, 3}}, /* Mapping */
    {0x66, 1, {{PartyStatDexterity, 0}}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {0, 0, 3, 3, 0, 0, 5, 0, 0, 5}}, /* Navigation */
    {0x68, 2, {{PartyStatIntelligence, 15}, {PartyStatCharisma, 85}}, {-1, -1, -1, -1, 40, -1, -1, 40, -1, 0}, {0, -5, 5, 0, 0, 0, 0, 0, 0, 0}}, /* Bartering */
    {0x6A, 2, {{PartyStatDexterity, 80}, {PartyStatIntelligence, 20}}, {-1, -1, -1, -1, -1, -1, 0, -1, -1, -1}, {0, 0, 5, 3, -10, 0, 0, 0, 0, -5}}, /* Repair */
    {0x6C, 1, {{PartyStatDexterity, 0}}, {-1, -1, -1, -1, 0, -1, -1, -1, 0, -1}, {0, -10, 0, 5, 0, 0, 0, -5, 0, 0}}, /* Thievery */
    {0x6E, 2, {{PartyStatIntelligence, 70}, {PartyStatWisdom, 30}}, {-1, 0, -1, 0, -1, -1, -1, -1, -1, -1}, {0, 0, -5, 0, 0, 0, -5, 3, 5, -5}}, /* Linguistics */
};

const DerivedStatRule *partyDerivedStatRules(GameKind game, unsigned *count) {
    if (game == GameYendor3) {
        *count = sizeof(g_derivedRulesYendor3) / sizeof(g_derivedRulesYendor3[0]);
        return g_derivedRulesYendor3;
    }
    *count = sizeof(g_derivedRulesYendor2) / sizeof(g_derivedRulesYendor2[0]);
    return g_derivedRulesYendor2;
}

/* ScaleByPercentRounded (yendor2.asm:38474): the multiply and the +50 wrap at 16 bits. */
static uint16_t scalePercent(uint16_t value, unsigned percent) {
    uint16_t product = (uint16_t)(value * percent);
    return (uint16_t)((uint16_t)(product + 50) / 100);
}

static void setPair(uint8_t *record, unsigned currentOffset, uint16_t value) {
    partySetU16(record, currentOffset, value);
    partySetU16(record, currentOffset + PartyMaxStatOffset, value);
}

void partyApplyRolledAttributes(uint8_t *record, const uint8_t rolls[6]) {
    setPair(record, PartyFieldEquipRatingBase1, 0);
    setPair(record, PartyFieldEquipRatingBase3, 0);

    uint16_t strength = (uint16_t)(CharGenRollBase + rolls[0]);
    uint16_t dexterity = (uint16_t)(CharGenRollBase + rolls[1]);
    uint16_t intelligence = (uint16_t)(CharGenRollBase + rolls[2]);
    uint16_t wisdom = (uint16_t)(CharGenRollBase + rolls[3]);
    uint16_t charisma = (uint16_t)(CharGenRollBase + rolls[4]);
    uint16_t stamina = (uint16_t)(CharGenRollBase + rolls[5]);
    setPair(record, PartyFieldStats + PartyStatStrength * 2, strength);
    setPair(record, PartyFieldStats + PartyStatCarryCapacity * 2, (uint16_t)(10 * strength));
    setPair(record, PartyFieldStats + PartyStatDexterity * 2, dexterity);
    setPair(record, PartyFieldStats + PartyStatIntelligence * 2, intelligence);
    setPair(record, PartyFieldStats + PartyStatWisdom * 2, wisdom);
    setPair(record, PartyFieldStats + PartyStatCharisma * 2, charisma);
    setPair(record, PartyFieldStats + PartyStatStamina * 2, stamina);
    setPair(record, PartyFieldStats + PartyStatHitPoints * 2, scalePercent(stamina, 25));

    int16_t cls = (int16_t)partyGetU16(record, PartyFieldClass);
    uint16_t base = 0, bonus = 0;
    if (cls >= 4) {
        switch (cls) {
        case 4:
            base = wisdom;
            bonus = 10;
            break;
        case 8:
            base = (uint16_t)(scalePercent(intelligence, 75) + scalePercent(wisdom, 25));
            bonus = 5;
            break;
        case 5:
            base = (uint16_t)(scalePercent(wisdom, 75) + scalePercent(intelligence, 25));
            bonus = 5;
            break;
        case 6:
            base = scalePercent(wisdom, 50);
            bonus = (uint16_t)(base - (wisdom & 1));
            break;
        case 9:
            base = scalePercent(intelligence, 50);
            bonus = (uint16_t)(base - (wisdom & 1));
            break;
        default:
            base = intelligence;
            bonus = 10;
            break;
        }
    }
    setPair(record, PartyFieldStats + PartyStatMagicPoints * 2, (uint16_t)(base >> 2));
    setPair(record, PartyFieldStats + PartyStatCasting * 2, (uint16_t)(base + bonus));
}

unsigned partyApplyStartingAbilities(uint8_t *record, GameKind game) {
    static const uint8_t kStartFlags[6][2] = {{1, 3}, {1, 2}, {1, 0}, {2, 3}, {1, 2}, {2, 0}};
    unsigned cls = partyGetU16(record, PartyFieldClass);
    if (partyGetStatMax(record, PartyStatMagicPoints) == 0 || cls < 4 || cls > 9) {
        return 0;
    }
    if (game != GameYendor3) {
        partySetU16(record, PartyFieldStatusFlags,
                    (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) | partyClassSecondaryBit(cls)));
    }
    unsigned set = 0;
    for (unsigned j = 0; j < 2 && kStartFlags[cls - 4][j] != 0; j++) {
        flagBankSet(record + PartyFieldFlagBankCA, 16, kStartFlags[cls - 4][j]);
        set++;
    }
    return set;
}

void partyBeginClassSelection(uint8_t *record) {
    partySetU16(record, PartyFieldStatusFlags, (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) & 0xFFC0));
    memset(record + PartyFieldFlagBankCA, 0, 16 * 2);
}

void partyRerollAttributes(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng) {
    partyRollAttributes(record, rng);
    partyComputeDerivedStats(record, game);
    partyRecomputeEquipmentStatBonuses(record, catalog, game);
}

void partyChooseClass(uint8_t *record, unsigned classBase, GameKind game, const ItemCatalog *catalog, RandomState *rng) {
    partySetU16(record, PartyFieldClass, (uint16_t)classBase);
    partySetU16(record, PartyFieldLevel, 1);
    partyRerollAttributes(record, game, catalog, rng);
}

void partyRollAttributes(uint8_t *record, RandomState *rng) {
    uint8_t rolls[6];
    for (unsigned i = 0; i < 6; i++) {
        rolls[i] = (uint8_t)randomInRange(rng, CharGenRollBound);
    }
    partyApplyRolledAttributes(record, rolls);
}

void partyComputeDerivedStats(uint8_t *record, GameKind game) {
    unsigned count;
    const DerivedStatRule *rules = partyDerivedStatRules(game, &count);
    uint16_t cls = partyGetU16(record, PartyFieldClass);
    unsigned column = cls <= 9 ? cls : 0;
    for (unsigned i = 0; i < count; i++) {
        const DerivedStatRule *rule = &rules[i];
        uint16_t value;
        if (rule->flat[column] >= 0) {
            value = (uint16_t)rule->flat[column];
        } else {
            value = (uint16_t)(int16_t)rule->bonus[column];
            for (unsigned t = 0; t < rule->termCount; t++) {
                uint16_t attribute = partyGetStat(record, rule->terms[t].attribute);
                value = (uint16_t)(value + (rule->terms[t].percent == 0 ? attribute : scalePercent(attribute, rule->terms[t].percent)));
            }
        }
        setPair(record, rule->recordOffset, value);
    }
}

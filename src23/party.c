#include "party.h"

#include <string.h>

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

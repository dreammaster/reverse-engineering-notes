#include "consumable.h"

#include "party.h"

RestorativeKind partyRestorativeForItem(GameKind game, unsigned itemId) {
    static const unsigned ids2[] = {0x12, 0x13, 0x14, 0x1D, 0x17, 0x18};
    static const unsigned ids3[] = {0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    const unsigned *ids = game == GameYendor2 ? ids2 : ids3;
    for (unsigned i = 0; i < 6; i++) {
        if (ids[i] == itemId) {
            return (RestorativeKind)(RestorativeHpQuarter + i);
        }
    }
    return RestorativeNone;
}

bool partyUseRestorative(RestorativeKind kind, uint8_t *record) {
    int16_t hp = (int16_t)partyGetStat(record, PartyStatHitPoints);
    int16_t hpMax = (int16_t)partyGetStatMax(record, PartyStatHitPoints);
    int16_t mp = (int16_t)partyGetStat(record, PartyStatMagicPoints);
    int16_t mpMax = (int16_t)partyGetStatMax(record, PartyStatMagicPoints);

    switch (kind) {
    case RestorativeHpQuarter:
    case RestorativeHpHalf: {
        if (hpMax <= hp) {
            return false;
        }
        int gain = kind == RestorativeHpQuarter ? (uint16_t)hpMax >> 2 : (uint16_t)hpMax >> 1;
        int value = hp + gain;
        partySetStat(record, PartyStatHitPoints, (uint16_t)(value > hpMax ? hpMax : value));
        return true;
    }
    case RestorativeHpFull:
        if (hpMax <= hp) {
            return false;
        }
        partySetStat(record, PartyStatHitPoints, (uint16_t)hpMax);
        return true;
    case RestorativeMpHalf: {
        if (mpMax == 0 || mpMax <= mp) {
            return false;
        }
        int value = mp + ((uint16_t)mpMax >> 1);
        partySetStat(record, PartyStatMagicPoints, (uint16_t)(value > mpMax ? mpMax : value));
        return true;
    }
    case RestorativeMpFull:
        if (mpMax == 0 || mpMax <= mp) {
            return false;
        }
        partySetStat(record, PartyStatMagicPoints, (uint16_t)mpMax);
        return true;
    case RestorativeCure: {
        uint16_t status = partyGetU16(record, PartyFieldStatusFlags);
        if (!(status & 0xE000)) {
            return false;
        }
        partySetU16(record, PartyFieldStatusFlags, (uint16_t)(status & 0x1FFF));
        return true;
    }
    case RestorativeNone:
        break;
    }
    return false;
}

unsigned alchemyYieldDivisor(uint16_t chemistry) {
    int16_t skill = (int16_t)chemistry;
    if (skill >= 110) {
        return 2;
    }
    if (skill >= 95) {
        return 4;
    }
    if (skill >= 80) {
        return 5;
    }
    return 10;
}

AlchemyResult partyTransmuteOre(const uint8_t *partyRecord, Bcd4 source, Bcd4 destination, unsigned *consumed,
                                unsigned *yield) {
    uint16_t chemistry = partyGetStat(partyRecord, PartyStatChemistry);
    *consumed = 0;
    *yield = 0;
    if ((int16_t)chemistry < 0x41) {
        return AlchemySkillTooLow;
    }
    if (!bcd4AtLeastU16(source, 10)) {
        return AlchemyTooFewUnits;
    }
    unsigned amount = 100;
    if (!bcd4AtLeastU16(source, 100)) {
        amount = (unsigned)((source[3] >> 4) * 10 + (source[3] & 0x0F));
    }
    unsigned out = amount / alchemyYieldDivisor(chemistry);
    bcd4SubU16(source, (uint16_t)amount);
    bcd4AddU16(destination, (uint16_t)out);
    *consumed = amount;
    *yield = out;
    return AlchemyDone;
}

bool partyIsPercentRestorativeItem(GameKind game, unsigned itemId) {
    return game == GameYendor2 ? itemId >= 0x36 && itemId <= 0x46 : itemId >= 0x1F && itemId <= 0x20;
}

static uint16_t percentOf(uint16_t value, unsigned percent) {
    uint32_t product = (uint32_t)value * (uint16_t)percent;
    uint32_t low = ((product & 0xFFFF) + 0x32) & 0xFFFF;
    return (uint16_t)(((product & 0xFFFF0000u) | low) / 100);
}

PercentRestoreResult partyUsePercentRestorative(GameKind game, uint8_t *record, bool restoresMp, unsigned percent) {
    if (game == GameYendor3) {
        restoresMp = true;
    }
    PartyStat stat = restoresMp ? PartyStatMagicPoints : PartyStatHitPoints;
    if (restoresMp) {
        int classId = (int16_t)partyGetU16(record, PartyFieldClass);
        if (classId > 9) {
            classId -= 10;
            if (classId > 9) {
                classId -= 10;
            }
        }
        if (classId < 4) {
            partySetU16(record, PartyFieldStatusFlags, (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) | PartyStatusSick));
            return PercentRestoreMadeSick;
        }
    }
    int16_t max = (int16_t)partyGetStatMax(record, stat);
    int16_t value = (int16_t)(percentOf((uint16_t)max, percent) + partyGetStat(record, stat));
    partySetStat(record, stat, (uint16_t)(value > max ? max : value));
    return PercentRestoreApplied;
}

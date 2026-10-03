#include "dialogservice.h"

#include <string.h>

#include "bcd4.h"
#include "globalflags.h"
#include "party.h"

static uint16_t clampToTypeCap(uint16_t statOffset, uint16_t value) {
    int16_t cap = (statOffset == 0x52 || statOffset == 0x54) ? 0x270F : 0x3E7;
    return (int16_t)value > cap ? (uint16_t)cap : value;
}

DialogTomeResult dialogApplyAttributeTome(const uint8_t *npc, SaveGame *save, uint8_t *globalFlags, size_t flagsSize) {
    DialogTomeResult result = {false, 0};
    unsigned flag = dialogGetU16(npc, DialogNpcOneTimeFlag);
    if (globalFlagTest(globalFlags, flagsSize, flag)) {
        return result;
    }
    result.applied = true;
    globalFlagSet(globalFlags, flagsSize, flag);

    uint16_t statOffset = dialogGetU16(npc, DialogNpcParamA);
    uint16_t amount = dialogGetU16(npc, DialogNpcParamB);
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead)) {
            continue;
        }
        int16_t current = (int16_t)partyGetU16(record, statOffset);
        if (current <= 0) {
            continue;
        }
        partySetU16(record, statOffset, clampToTypeCap(statOffset, (uint16_t)(current + amount)));
        partySetU16(record, statOffset + 0x40,
                    clampToTypeCap(statOffset, (uint16_t)(partyGetU16(record, statOffset + 0x40) + amount)));
        partyRefreshCarryCapacityAndAttributeBonuses(record);
        result.members++;
    }
    return result;
}

DialogTomeResult dialogApplyExperienceTome(const uint8_t *npc, SaveGame *save, GameKind game, uint8_t *globalFlags,
                                           size_t flagsSize) {
    DialogTomeResult result = {false, 0};
    unsigned flag = dialogGetU16(npc, DialogNpcOneTimeFlag);
    if (globalFlagTest(globalFlags, flagsSize, flag)) {
        return result;
    }
    result.applied = true;
    globalFlagSet(globalFlags, flagsSize, flag);

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead)) {
            continue;
        }
        bcd4Add(record + PartyFieldExperience, npc + DialogNpcParamA);
        partyCheckForLevelUp(record, game);
        result.members++;
    }
    return result;
}

void dialogClassifyCondition(DialogState *state, const uint8_t *partyRecord) {
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    uint16_t bits = 0;
    unsigned tiers = 0;
    if (status & PartyStatusDead) {
        bits |= 0x2000;
    }
    if (status & 0xFF80) {
        bits |= 0x4000;
        tiers++;
    }
    if ((int16_t)partyGetStat(partyRecord, PartyStatHitPoints) < (int16_t)partyGetStatMax(partyRecord, PartyStatHitPoints)) {
        bits |= 0x8000;
        tiers++;
    }
    if (tiers > 1) {
        bits |= 0x1000;
    } else if (tiers < 1) {
        bits |= 0x200;
    }
    state->availA = (uint16_t)((state->availA & 0x3FF) | bits);
}

unsigned dialogAfflictionCost(const uint8_t *partyRecord) {
    static const struct {
        uint16_t bit;
        unsigned cost;
    } table[] = {{0x8000, 5},  {0x4000, 10}, {0x2000, 20}, {0x1000, 40}, {0x0800, 50},
                 {0x0400, 60}, {0x0200, 20}, {0x0100, 30}, {0x0080, 40}};
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    unsigned total = 0;
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (status & table[i].bit) {
            total += table[i].cost;
        }
    }
    return total;
}

void dialogHealingCost(const uint8_t *npc, unsigned type, const DialogState *state, const uint8_t *partyRecord,
                       Bcd4 cost) {
    unsigned base = 0;
    if (type & DialogHealFull) {
        if (state->availA & 0x4000) {
            base = dialogAfflictionCost(partyRecord);
        }
        if (state->availA & 0x8000) {
            base += 20;
        }
        if (state->availA & 0x2000) {
            base += 100;
        }
    } else if (type & DialogHealRevive) {
        base = 100;
    } else if (type & DialogHealCure) {
        base = dialogAfflictionCost(partyRecord);
    } else if (type & DialogHealHp) {
        base = 20;
    }
    uint16_t perLevel = (uint16_t)(base * dialogGetU16(npc, DialogNpcPriceMultiplier));
    memset(cost, 0, sizeof(Bcd4));
    for (unsigned i = 0; i < partyGetU16(partyRecord, PartyFieldLevel); i++) {
        bcd4AddU16(cost, perLevel);
    }
}

bool dialogApplyHealing(unsigned type, const Bcd4 cost, Bcd4 gold, uint8_t *partyRecord, GameKind game) {
    if (bcd4Compare(gold, cost) < 0) {
        return false;
    }
    bcd4Sub(gold, cost);
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    if (type & DialogHealRevive) {
        status = (uint16_t)(status & ~PartyStatusDead);
        partySetStat(partyRecord, PartyStatHitPoints, 2);
    }
    if (type & DialogHealCure) {
        status = (uint16_t)(status & 0x7F);
    }
    if (type & DialogHealHp) {
        partySetStat(partyRecord, PartyStatHitPoints, partyGetStatMax(partyRecord, PartyStatHitPoints));
    }
    if (type & DialogHealFull) {
        partySetStat(partyRecord, PartyStatHitPoints, partyGetStatMax(partyRecord, PartyStatHitPoints));
        status = (uint16_t)(status & 0x3F);
    }
    partySetU16(partyRecord, PartyFieldStatusFlags, status);
    partyCheckForLevelUp(partyRecord, game);
    return true;
}

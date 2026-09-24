#include "combat.h"

#include <string.h>

#include "party.h"

static bool partySlotIsUsable(SaveGame *save, unsigned slot, uint8_t **outRecord) {
    uint16_t id = saveGetPartySlot(save, slot);
    if (id == 0) {
        return false;
    }
    uint8_t *record = saveGamePartyRecordById(save, id);
    if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
        return false;
    }
    if (outRecord) {
        *outRecord = record;
    }
    return true;
}

static void insertSorted(CombatTurnOrderEntry *out, unsigned count, CombatTurnOrderEntry entry) {
    unsigned i = count;
    while (i > 0 && out[i - 1].speed < entry.speed) {
        out[i] = out[i - 1];
        i--;
    }
    out[i] = entry;
}

unsigned combatBuildTurnOrder(SaveGame *save, const uint8_t *monsterSlots, RandomState *rng,
                               CombatTurnOrderEntry out[CombatTurnOrderCapacity],
                               uint16_t monsterTargets[CombatMonsterSlotCount]) {
    unsigned count = 0;

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint8_t *record;
        if (!partySlotIsUsable(save, slot, &record)) {
            continue;
        }
        CombatTurnOrderEntry entry;
        entry.isMonster = false;
        entry.index = slot;
        entry.speed = partyGetStat(record, PartyStatDexterity);
        insertSorted(out, count, entry);
        count++;
    }

    for (unsigned slot = 0; slot < CombatMonsterSlotCount; slot++) {
        monsterTargets[slot] = 0;
        const uint8_t *record = monsterSlots + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0) {
            continue;
        }
        CombatTurnOrderEntry entry;
        entry.isMonster = true;
        entry.index = slot;
        entry.speed = monsterGetU16(record, MonsterFieldDexterity);
        insertSorted(out, count, entry);
        count++;

        for (unsigned attempt = 0; attempt < 64; attempt++) {
            unsigned candidate = randomInRange(rng, 3);
            uint8_t *candidateRecord;
            if (partySlotIsUsable(save, candidate, &candidateRecord)) {
                monsterTargets[slot] = (uint16_t)(saveGetPartySlot(save, candidate));
                break;
            }
        }
    }

    return count;
}

bool combatSelectActiveMonster(const CombatTurnOrderEntry *turnOrder, unsigned count,
                                const bool defeated[CombatMonsterSlotCount], unsigned *outMonsterSlot) {
    for (unsigned i = 0; i < count; i++) {
        if (turnOrder[i].isMonster && !defeated[turnOrder[i].index]) {
            *outMonsterSlot = turnOrder[i].index;
            return true;
        }
    }
    return false;
}

CombatRoundOutcome combatProcessRound(uint8_t *monsterSlots, CombatTurnOrderEntry *turnOrder, unsigned turnOrderCount,
                                       bool defeated[CombatMonsterSlotCount], unsigned *turnCursor,
                                       MonsterRewardStaging *staging, uint8_t *globalFlags, size_t globalFlagsSize) {
    bool anyAlive = false;

    for (unsigned slot = 0; slot < CombatMonsterSlotCount; slot++) {
        uint8_t *record = monsterSlots + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0) {
            continue;
        }
        if ((int16_t)monsterGetU16(record, MonsterFieldHealth) > 0) {
            anyAlive = true;
            continue;
        }
        defeated[slot] = true;
        monsterGrantRewards(staging, record, globalFlags, globalFlagsSize);
        memset(record, 0, MonsterRecordSize);
    }

    if (!anyAlive) {
        return CombatRoundNoMonstersLeft;
    }

    for (unsigned i = *turnCursor + 1; i < turnOrderCount; i++) {
        if (turnOrder[i].isMonster && defeated[turnOrder[i].index]) {
            continue;
        }
        *turnCursor = i;
        return CombatRoundContinue;
    }
    return CombatRoundNewRound;
}

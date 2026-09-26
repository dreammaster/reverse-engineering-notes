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

uint16_t combatResolveAttack(uint16_t defense, uint16_t accuracy, uint16_t power, RandomState *rng) {
    if (power == 0) {
        return 0;
    }
    int16_t diff = (int16_t)(accuracy - defense);
    if (diff < 0) {
        return 0;
    }
    uint16_t roll = randomInRange(rng, 55);
    if (diff < (int16_t)roll) {
        return 0;
    }
    uint32_t damage = ((uint32_t)power * (uint32_t)diff + 50u) / 100u;
    return damage > 0 ? (uint16_t)damage : 1;
}

bool combatFailsSavingThrow(int16_t defenderStat, int16_t threshold, int16_t bonus, RandomState *rng) {
    int32_t chance = (int32_t)5 * (defenderStat - threshold) + bonus;
    if (chance < 5) {
        chance = 5;
    }
    uint16_t roll = randomInRange(rng, 100);
    return (int32_t)roll > chance;
}

void combatApplyEffect(uint8_t *defenderRecord, SaveGame *save, EffectSpend spend, uint16_t amount,
                        const Bcd4 materialAmount, uint16_t inflictedStatus) {
    switch (spend) {
    case EffectSpendHp:
        partyDeductHp(defenderRecord, amount);
        break;
    case EffectSpendMp:
        partyDeductMp(defenderRecord, amount);
        break;
    case EffectSpendHpAndMp:
        partyDeductHp(defenderRecord, amount);
        partyDeductMp(defenderRecord, amount);
        break;
    case EffectSpendGold:
        bcd4SubClamped(saveHeaderBcd4(save, SaveHeaderGold), materialAmount);
        break;
    case EffectSpendOre1:
        bcd4SubClamped(saveHeaderBcd4(save, SaveHeaderOreCounter1), materialAmount);
        break;
    case EffectSpendOre2:
        bcd4SubClamped(saveHeaderBcd4(save, SaveHeaderOreCounter2), materialAmount);
        break;
    case EffectSpendNone:
        break;
    }

    if (inflictedStatus != 0) {
        partySetU16(defenderRecord, PartyFieldStatusFlags,
                    (uint16_t)(partyGetU16(defenderRecord, PartyFieldStatusFlags) | inflictedStatus));
    }
}

CombatEffectSelection combatSelectTrapEffectVariant(const uint8_t *attackerRecord, RandomState *rng) {
    CombatEffectSelection selection;
    selection.effectId = monsterGetU16(attackerRecord, MonsterFieldAttackEffect);
    selection.isSpecial = false;

    if (monsterGetU16(attackerRecord, MonsterFieldState) & MonsterStateSpecialAttackDisabled) {
        return selection;
    }
    unsigned special = monsterGetU16(attackerRecord, MonsterFieldSpecialAttack);
    if (special == 0) {
        return selection;
    }
    if (randomInRange(rng, 100) < 25) {
        selection.effectId = special;
        selection.isSpecial = true;
    }
    return selection;
}

static bool bcd4IsZero(const uint8_t *value) {
    return value[0] == 0 && value[1] == 0 && value[2] == 0 && value[3] == 0;
}

CombatAttackerAction combatResolveAttackerAction(const uint8_t *attackerRecord, const uint8_t *defenderRecord,
                                                  const ItemCatalog *catalog, bool isSpecial, RandomState *rng) {
    CombatAttackerAction action;
    memset(&action, 0, sizeof(action));

    uint16_t attackerFlags = monsterGetU16(attackerRecord, MonsterFieldFlags);
    const uint8_t *goldTheftAmount = attackerRecord + MonsterFieldGoldTheftAmount;

    if (isSpecial && (attackerFlags & MonsterFlagSpecialMask)) {
        if (attackerFlags & MonsterFlagCorrodeWeaponSlot) {
            action.equipSlotOffset = 0x13A;
        } else if (attackerFlags & MonsterFlagCorrodeSecondSlot) {
            action.equipSlotOffset = 0x142;
        } else {
            action.equipSlotOffset = 0x146;
        }

        int16_t bonus = (int16_t)(partyGetStat(defenderRecord, PartyStatSurvival) / 2);
        bool failed = combatFailsSavingThrow((int16_t)partyGetU16(defenderRecord, PartyFieldLevel),
                                              (int16_t)monsterGetU16(attackerRecord, MonsterFieldSaveDifficulty),
                                              bonus, rng);
        if (!failed) {
            return action;
        }

        action.equippedItemId = partyGetU16(defenderRecord, action.equipSlotOffset);
        if (action.equippedItemId == 0) {
            return action;
        }
        const uint8_t *itemRecord = itemCatalogRecord(catalog, action.equippedItemId);
        if (!itemRecord) {
            return action;
        }
        action.corrosionReplacementId = itemCorrosionReplacement(catalog, itemRecord);
        if (action.corrosionReplacementId == 0) {
            return action;
        }
        action.outcome = CombatAttackCorrosion;
        return action;
    }

    if (isSpecial && !bcd4IsZero(goldTheftAmount)) {
        bool failed = combatFailsSavingThrow((int16_t)partyGetU16(defenderRecord, PartyFieldLevel),
                                              (int16_t)monsterGetU16(attackerRecord, MonsterFieldSaveDifficulty),
                                              (int16_t)partyGetStat(defenderRecord, PartyStatSurvival), rng);
        if (!failed) {
            return action;
        }
        memcpy(action.goldAmount, goldTheftAmount, sizeof(Bcd4));
        action.outcome = CombatAttackStatusEffect;
        return action;
    }

    uint16_t damage = combatResolveAttack(partyGetStat(defenderRecord, PartyStatEquipRating5),
                                           monsterGetU16(attackerRecord, MonsterFieldAccuracy),
                                           monsterGetU16(attackerRecord, MonsterFieldDamage), rng);
    if (damage != 0) {
        action.outcome = CombatAttackDamage;
        action.damage = damage;
    }
    return action;
}

static void combatApplyTrapEffectToRecipient(uint8_t *recipientRecord, SaveGame *save, const EffectDef *def,
                                              unsigned threshold, RandomState *rng) {
    uint16_t level = partyGetU16(recipientRecord, PartyFieldLevel);
    uint16_t magnitude = effectRollsMagnitude(def) ? effectRollMagnitude(def, level, rng) : 0;

    /*
     * Matches RollEffectResistance's own double early-out exactly
     * (yendor2.asm:14127): no roll at all -- not just "the roll
     * wouldn't change the outcome" -- when the effect inflicts nothing
     * (effectInflictedStatus == 0) or doesn't gate on one
     * (EffectModeRollResistance unset). Skipping the roll here rather
     * than always calling combatFailsSavingThrow keeps this
     * composition's RNG draw count identical to the original's for
     * every effect definition, not just outcome-identical.
     */
    bool failed = false;
    if (effectInflictedStatus(def) != 0 && (def->modeFlags & EffectModeRollResistance)) {
        failed = combatFailsSavingThrow((int16_t)level, (int16_t)threshold,
                                         (int16_t)effectResistanceBonus(def, recipientRecord), rng);
    }
    uint16_t inflicted = effectResolveInflictedStatus(def, failed);
    static const Bcd4 zeroMaterial = {0, 0, 0, 0};
    combatApplyEffect(recipientRecord, save, effectSpend(def), magnitude, zeroMaterial, inflicted);
}

CombatSavingThrowTrapOutcome combatApplySavingThrowTrap(uint16_t packedValue, uint8_t *actingRecord,
                                                          SaveGame *save, GameKind game, RandomState *rng) {
    PartySavingThrowEffect decoded;
    if (!partyDecodeSavingThrowEffect(packedValue, &decoded)) {
        return CombatSavingThrowTrapNone;
    }

    bool triggered = combatFailsSavingThrow((int16_t)partyGetU16(actingRecord, PartyFieldLevel),
                                             (int16_t)decoded.threshold,
                                             (int16_t)partyGetStat(actingRecord, PartyStatThievery), rng);
    if (!triggered) {
        return CombatSavingThrowTrapNone;
    }

    EffectDef def;
    if (!effectGetDef(game, decoded.effectId, &def)) {
        return CombatSavingThrowTrapNone;
    }

    if (!decoded.wholeParty) {
        combatApplyTrapEffectToRecipient(actingRecord, save, &def, decoded.threshold, rng);
        return CombatSavingThrowTrapSingle;
    }

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
            continue;
        }
        combatApplyTrapEffectToRecipient(record, save, &def, decoded.threshold, rng);
    }
    return CombatSavingThrowTrapParty;
}

CombatEncodedItemEffectValue combatResolveEncodedItemEffectValue(bool curseGateActive, const uint8_t *actingRecord,
                                                                   uint16_t inflictedStatus, uint16_t magnitude) {
    CombatEncodedItemEffectValue value;
    if (curseGateActive && (partyGetU16(actingRecord, PartyFieldStatusFlags) & PartyStatusCursed)) {
        value.inflictedStatus = 0;
        value.magnitude = 0;
        return value;
    }
    value.inflictedStatus = inflictedStatus;
    value.magnitude = magnitude;
    return value;
}

void combatApplyEncodedItemEffectSingle(uint8_t *actingRecord, SaveGame *save, unsigned effectId, GameKind game,
                                          CombatEncodedItemEffectValue value) {
    EffectDef def;
    if (!effectGetDef(game, effectId, &def)) {
        return;
    }
    static const Bcd4 zeroMaterial = {0, 0, 0, 0};
    combatApplyEffect(actingRecord, save, effectSpend(&def), value.magnitude, zeroMaterial, value.inflictedStatus);
}

void combatApplyEncodedItemEffectParty(SaveGame *save, unsigned effectId, GameKind game,
                                        CombatEncodedItemEffectValue value) {
    EffectDef def;
    if (!effectGetDef(game, effectId, &def)) {
        return;
    }
    static const Bcd4 zeroMaterial = {0, 0, 0, 0};
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record) {
            continue;
        }
        if (game == GameYendor3 && (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusCursed)) {
            continue;
        }
        combatApplyEffect(record, save, effectSpend(&def), value.magnitude, zeroMaterial, value.inflictedStatus);
    }
}

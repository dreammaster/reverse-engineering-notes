#include "combat.h"

#include <string.h>

#include "party.h"
#include "spellrecord.h"

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

void combatApplyCorrosion(uint8_t *defenderRecord, const ItemCatalog *catalog, GameKind game,
                           const CombatEffectSelection *selection, const CombatAttackerAction *action) {
    if (action->outcome != CombatAttackCorrosion) {
        return;
    }
    EffectDef def;
    if (!effectGetDef(game, selection->effectId, &def)) {
        return;
    }
    partyHandleIconBarItemExpiry(defenderRecord, catalog, def.modeFlags, action->equippedItemId,
                                  action->corrosionReplacementId, action->equipSlotOffset);
}

CombatTargetAttackResult combatApplyTargetResistances(uint8_t *targetRecord, uint16_t damage, uint16_t attackFlags,
                                                        uint16_t resistanceFlags, uint16_t drainAmount) {
    CombatTargetAttackResult result;
    result.damage = damage;
    result.statusFlags = 0;

    uint16_t immunities = monsterGetU16(targetRecord, MonsterFieldImmunities);

    if (attackFlags & 0xFC00u) {
        static const uint16_t statusBits[] = {0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400};
        for (size_t i = 0; i < sizeof(statusBits) / sizeof(statusBits[0]); i++) {
            if ((attackFlags & statusBits[i]) && !(immunities & statusBits[i])) {
                result.statusFlags = (uint16_t)(result.statusFlags | statusBits[i]);
            }
        }
    }

    static const uint16_t negateBits[] = {0x0008, 0x0004, 0x0002, 0x0001, 0x0010};
    for (size_t i = 0; i < sizeof(negateBits) / sizeof(negateBits[0]); i++) {
        if ((attackFlags & negateBits[i]) && (immunities & negateBits[i])) {
            result.damage = 0;
            return result;
        }
    }

    uint16_t resistances = monsterGetU16(targetRecord, MonsterFieldResistances);
    static const uint16_t resistBits[] = {0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400};
    for (size_t i = 0; i < sizeof(resistBits) / sizeof(resistBits[0]); i++) {
        if ((resistanceFlags & resistBits[i]) && (resistances & resistBits[i])) {
            result.damage = (uint16_t)(result.damage >> 1);
            return result;
        }
    }
    if (resistanceFlags & 0x0200u) {
        if (resistances & 0x0200u) {
            result.damage = (uint16_t)(result.damage >> 1);
        }
        return result;
    }

    if (attackFlags & 0x03E0u) {
        unsigned fieldOffset;
        if (attackFlags & 0x0200u) {
            fieldOffset = MonsterFieldHealth;
        } else if (attackFlags & 0x0100u) {
            fieldOffset = MonsterFieldAccuracy;
        } else if (attackFlags & 0x0080u) {
            fieldOffset = MonsterFieldDexterity;
        } else if (attackFlags & 0x0040u) {
            fieldOffset = MonsterFieldAbsorption;
        } else {
            fieldOffset = MonsterFieldDamage;
        }
        int32_t value = (int32_t)monsterGetU16(targetRecord, fieldOffset) - (int32_t)drainAmount;
        if (value < 0) {
            value = 0;
        }
        monsterSetU16(targetRecord, fieldOffset, (uint16_t)value);
    }
    return result;
}

CombatSpellAttackResult combatResolveSpellAttack(const uint8_t *targetRecord, const uint8_t *casterRecord,
                                                   const uint8_t *spellRecord, bool alreadyResolved, RandomState *rng) {
    CombatSpellAttackResult result;
    result.damage = 0;
    result.statusFlags = 0;
    result.hasEffect = false;

    uint16_t resistFlags = spellGetU16(spellRecord, SpellFieldResistFlags);
    if (resistFlags & SpellResistTypeRestricted) {
        uint16_t wantedType = spellGetU16(spellRecord, SpellFieldTargetTypeId);
        if (monsterGetU16(targetRecord, MonsterFieldUnknown4E) != wantedType) {
            return result;
        }
    }

    uint16_t damage;
    if (alreadyResolved) {
        damage = spellGetU16(spellRecord, SpellFieldAttackMagnitude);
    } else {
        uint16_t defense = monsterGetU16(targetRecord, MonsterFieldAbsorption);
        uint16_t accuracy = partyGetStat(casterRecord, PartyStatCasting);
        uint16_t power = spellGetU16(spellRecord, SpellFieldAttackMagnitude);
        damage = combatResolveAttack(defense, accuracy, power, rng);
        if (damage == 0) {
            return result;
        }
    }

    uint16_t attackFlags = spellGetU16(spellRecord, SpellFieldAttackFlags);
    uint16_t drainAmount = spellGetU16(spellRecord, SpellFieldDrainAmount);
    /* combatApplyTargetResistances mutates targetRecord directly for its own drain effect -- see its own doc comment. */
    CombatTargetAttackResult filtered =
        combatApplyTargetResistances((uint8_t *)targetRecord, damage, attackFlags, resistFlags, drainAmount);

    result.damage = filtered.damage;
    result.statusFlags = filtered.statusFlags;
    result.hasEffect = filtered.damage != 0 || filtered.statusFlags != 0;
    return result;
}

void combatApplySpellAttack(uint8_t *targetRecord, const uint8_t *spellRecord, CombatSpellAttackResult result) {
    if (!result.hasEffect) {
        return;
    }
    uint16_t resistFlags = spellGetU16(spellRecord, SpellFieldResistFlags);
    uint16_t damage = result.damage;
    if (resistFlags & SpellResistHalfTargetDamage) {
        damage = (uint16_t)(monsterGetU16(targetRecord, MonsterFieldDamage) >> 1);
    }

    monsterSetU16(targetRecord, MonsterFieldState,
                  (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) | MonsterStateAware | MonsterStateHitFlashPending));
    monsterSetU16(targetRecord, MonsterFieldHealth, (uint16_t)(monsterGetU16(targetRecord, MonsterFieldHealth) - damage));

    if (result.statusFlags != 0) {
        monsterSetU16(targetRecord, MonsterFieldState,
                      (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) | result.statusFlags));
        if (spellGetU16(spellRecord, SpellFieldFlagsA) & SpellFlagsAPersistAffliction) {
            monsterSetU16(targetRecord, MonsterFieldImmunities,
                          (uint16_t)(monsterGetU16(targetRecord, MonsterFieldImmunities) | result.statusFlags));
        }
        monsterSetU16(targetRecord, MonsterFieldTickAmount, spellGetU16(spellRecord, SpellFieldTickAmount));
        monsterSetU16(targetRecord, MonsterFieldTickCountdown, spellGetU16(spellRecord, SpellFieldTickCountdown));
    }

    if (resistFlags & SpellResistClearAware) {
        monsterSetU16(targetRecord, MonsterFieldState,
                      (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) & (uint16_t)~MonsterStateAware));
    }
}

void combatMarkSpellAttackHit(uint8_t *targetRecord, const uint8_t *spellRecord) {
    monsterSetU16(targetRecord, MonsterFieldLastAttackMarker, spellGetU16(spellRecord, SpellFieldInflictedMagnitude));
}

CombatSpellAreaAttackOutcome combatApplySpellAttackToActiveSlots(uint8_t *monsterSlots, const uint8_t *casterRecord,
                                                                    const uint8_t *spellRecord, bool alreadyResolved,
                                                                    RandomState *rng) {
    CombatSpellAreaAttackOutcome outcome;
    for (unsigned slot = 0; slot < MonsterActiveSlots; slot++) {
        uint8_t *record = monsterSlots + (size_t)slot * MonsterRecordSize;
        outcome.hit[slot] = false;
        if (monsterGetU16(record, MonsterFieldType) == 0 || (int16_t)monsterGetU16(record, MonsterFieldHealth) <= 0) {
            continue;
        }
        CombatSpellAttackResult result = combatResolveSpellAttack(record, casterRecord, spellRecord, alreadyResolved, rng);
        combatApplySpellAttack(record, spellRecord, result);
        if (result.hasEffect) {
            combatMarkSpellAttackHit(record, spellRecord);
            outcome.hit[slot] = true;
        }
    }
    return outcome;
}

void combatSaveLocationBookmark(uint8_t *partyRecord, unsigned bookmarkOffset, CombatLocationBookmark bookmark) {
    uint8_t *slot = partyRecord + bookmarkOffset;
    partySetU16(slot, 0x0, (uint16_t)bookmark.worldX);
    partySetU16(slot, 0x2, (uint16_t)bookmark.worldY);
    partySetU16(slot, 0x4, bookmark.facing);
    partySetU16(slot, 0x6, bookmark.renderA);
    partySetU16(slot, 0x8, bookmark.renderB);
    partySetU16(slot, 0xA, bookmark.renderC);
    partySetU16(slot, 0xC, bookmark.renderDLow3);
}

bool combatRestoreLocationBookmark(const uint8_t *partyRecord, unsigned bookmarkOffset, CombatLocationBookmark *out) {
    const uint8_t *slot = partyRecord + bookmarkOffset;
    if (partyGetU16(slot, 0x0) == 0) {
        return false;
    }
    out->worldX = (int16_t)partyGetU16(slot, 0x0);
    out->worldY = (int16_t)partyGetU16(slot, 0x2);
    out->facing = partyGetU16(slot, 0x4);
    out->renderA = partyGetU16(slot, 0x6);
    out->renderB = partyGetU16(slot, 0x8);
    out->renderC = partyGetU16(slot, 0xA);
    out->renderDLow3 = partyGetU16(slot, 0xC);
    return true;
}

CombatMapMonsterAttackOutcome combatApplyDamageToMapMonster(uint8_t *targetRecord, const uint8_t *casterRecord,
                                                              const uint8_t *spellRecord, bool alreadyResolved,
                                                              MonsterRewardStaging *staging, uint8_t *globalFlags,
                                                              size_t globalFlagsSize, DungeonGrid *grid,
                                                              RandomState *rng) {
    CombatMapMonsterAttackOutcome outcome;
    outcome.attack = combatResolveSpellAttack(targetRecord, casterRecord, spellRecord, alreadyResolved, rng);
    combatApplySpellAttack(targetRecord, spellRecord, outcome.attack);
    outcome.monsterDied = false;
    if (!outcome.attack.hasEffect) {
        return outcome;
    }
    combatMarkSpellAttackHit(targetRecord, spellRecord);
    if ((int16_t)monsterGetU16(targetRecord, MonsterFieldHealth) <= 0) {
        monsterGrantRewards(staging, targetRecord, globalFlags, globalFlagsSize);
        monsterPoolRemove(targetRecord, grid);
        outcome.monsterDied = true;
    }
    return outcome;
}

CombatProjectileHitOutcome combatApplyProjectileHit(uint8_t *targetRecord, const uint8_t *casterRecord,
                                                      const uint8_t *spellRecord, bool alreadyResolved,
                                                      MonsterRewardStaging *staging, uint8_t *globalFlags,
                                                      size_t globalFlagsSize, DungeonGrid *grid, RandomState *rng) {
    CombatProjectileHitOutcome outcome;
    outcome.attack = combatResolveSpellAttack(targetRecord, casterRecord, spellRecord, alreadyResolved, rng);
    combatApplySpellAttack(targetRecord, spellRecord, outcome.attack);
    outcome.monsterDied = false;
    outcome.continues = (spellGetU16(spellRecord, SpellFieldFlagsB) & SpellFlagsBPiercing) != 0;

    if (outcome.attack.hasEffect) {
        combatMarkSpellAttackHit(targetRecord, spellRecord);
        if (spellGetU16(spellRecord, SpellFieldAttackFlags) & SpellAttackTimedAffliction) {
            uint16_t overlay = monsterGetU16(targetRecord, MonsterFieldAnimSet) == 0xA
                                   ? spellGetU16(spellRecord, SpellFieldTickOverlayAnimSetA)
                                   : spellGetU16(spellRecord, SpellFieldTickOverlayDefault);
            monsterSetU16(targetRecord, MonsterFieldTickTarget, overlay);
            monsterSetU16(targetRecord, MonsterFieldTickAmount, spellGetU16(spellRecord, SpellFieldTickAmount));
            monsterSetU16(targetRecord, MonsterFieldState,
                          (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) | MonsterStateTimedAffliction));
            monsterSetU16(targetRecord, MonsterFieldTickCountdown, spellGetU16(spellRecord, SpellFieldTickCountdown));
            if (spellGetU16(spellRecord, SpellFieldFlagsA) & SpellFlagsAPersistAffliction) {
                monsterSetU16(targetRecord, MonsterFieldImmunities,
                              (uint16_t)(monsterGetU16(targetRecord, MonsterFieldImmunities) | SpellAttackTimedAffliction));
            }
        }
    }

    /* Only a plain miss from a non-piercing projectile skips the kill check (loc_2CACB jumps straight to the end). */
    if ((outcome.attack.hasEffect || outcome.continues) && (int16_t)monsterGetU16(targetRecord, MonsterFieldHealth) <= 0) {
        monsterGrantRewards(staging, targetRecord, globalFlags, globalFlagsSize);
        monsterPoolRemove(targetRecord, grid);
        outcome.monsterDied = true;
    }
    return outcome;
}

CombatSpellAttackResult combatApplySplashHit(uint8_t *targetRecord, const uint8_t *casterRecord,
                                               const uint8_t *spellRecord, bool alreadyResolved, RandomState *rng) {
    CombatSpellAttackResult result = combatResolveSpellAttack(targetRecord, casterRecord, spellRecord, alreadyResolved, rng);
    combatApplySpellAttack(targetRecord, spellRecord, result);
    if (!result.hasEffect) {
        return result;
    }

    uint16_t resistFlags = spellGetU16(spellRecord, SpellFieldResistFlags);
    /* g_stagedAttackDamage as ApplyAttackToTarget left it: the half-target-damage replacement persists there. */
    uint16_t damage = (resistFlags & SpellResistHalfTargetDamage)
                          ? (uint16_t)(monsterGetU16(targetRecord, MonsterFieldDamage) >> 1)
                          : result.damage;

    /* Idempotent re-filter of the staged status flags by immunity (the first pass already OR'd them in). */
    monsterSetU16(targetRecord, MonsterFieldState,
                  (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) |
                             (uint16_t)(~monsterGetU16(targetRecord, MonsterFieldImmunities) & result.statusFlags)));

    /* One halving per matching resistance bit (0x200-0x8000), compounding -- unlike the first pass's single halving. */
    uint16_t matching = (uint16_t)(resistFlags & 0xFE00u & monsterGetU16(targetRecord, MonsterFieldResistances));
    for (unsigned bit = 0; bit < 16; bit++) {
        if (matching & (1u << bit)) {
            damage = (uint16_t)(damage >> 1);
        }
    }

    int16_t health = (int16_t)((uint16_t)(monsterGetU16(targetRecord, MonsterFieldHealth) - damage));
    monsterSetU16(targetRecord, MonsterFieldHealth, health > 0 ? (uint16_t)health : 0);
    monsterSetU16(targetRecord, MonsterFieldState, (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) | 3u));
    if (resistFlags & SpellResistClearAware) {
        monsterSetU16(targetRecord, MonsterFieldState,
                      (uint16_t)(monsterGetU16(targetRecord, MonsterFieldState) & (uint16_t)~MonsterStateAware));
    }
    return result;
}

unsigned combatReapDeadMapMonsters(uint8_t *pool, MonsterRewardStaging *staging, uint8_t *globalFlags,
                                     size_t globalFlagsSize, DungeonGrid *grid) {
    unsigned reaped = 0;
    for (unsigned i = 0; i < MonsterPoolSize; i++) {
        uint8_t *record = pool + (size_t)i * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0 || (int16_t)monsterGetU16(record, MonsterFieldHealth) > 0) {
            continue;
        }
        monsterGrantRewards(staging, record, globalFlags, globalFlagsSize);
        monsterPoolRemove(record, grid);
        reaped++;
    }
    return reaped;
}

CombatScreenAttackOutcome combatApplyScreenWideAttack(uint8_t *monsterSlots, uint8_t *const rowMonsters[CombatScreenAttackRows],
                                                        const uint8_t *casterRecord, const uint8_t *spellRecord,
                                                        bool alreadyResolved, MonsterRewardStaging *staging,
                                                        uint8_t *globalFlags, size_t globalFlagsSize, DungeonGrid *grid,
                                                        RandomState *rng) {
    CombatScreenAttackOutcome outcome = {0, 0};
    if (monsterSlots) {
        for (unsigned slot = 0; slot < MonsterActiveSlots; slot++) {
            uint8_t *record = monsterSlots + (size_t)slot * MonsterRecordSize;
            if (monsterGetU16(record, MonsterFieldType) == 0) {
                continue;
            }
            CombatMapMonsterAttackOutcome r = combatApplyDamageToMapMonster(record, casterRecord, spellRecord, alreadyResolved,
                                                                              staging, globalFlags, globalFlagsSize, grid, rng);
            outcome.attacked++;
            outcome.killed += r.monsterDied ? 1 : 0;
        }
    }
    for (unsigned row = 0; row < CombatScreenAttackRows; row++) {
        if (!rowMonsters[row]) {
            continue;
        }
        CombatMapMonsterAttackOutcome r = combatApplyDamageToMapMonster(rowMonsters[row], casterRecord, spellRecord,
                                                                          alreadyResolved, staging, globalFlags,
                                                                          globalFlagsSize, grid, rng);
        outcome.attacked++;
        outcome.killed += r.monsterDied ? 1 : 0;
    }
    return outcome;
}

/*
 * One icon-bar effect slot through ApplyEffectAndDrawIconBar's own
 * RollEffectMagnitude/RollEffectResistance/ApplyEffectCost sequence.
 * presetAmount/presetStatus are the slot's +0x10/+0xE words as the caller
 * staged them: each roll is skipped when its word is already nonzero
 * ("already resolved"), exactly the original's early-outs, so a caller that
 * stages nothing (0, 0) gets both rolls.
 */
static void combatApplyEffectSlot(uint8_t *recipientRecord, SaveGame *save, const EffectDef *def, uint16_t presetAmount,
                                   uint16_t presetStatus, const Bcd4 material, unsigned threshold, RandomState *rng) {
    uint16_t level = partyGetU16(recipientRecord, PartyFieldLevel);
    uint16_t magnitude = presetAmount;
    if (magnitude == 0 && effectRollsMagnitude(def)) {
        magnitude = effectRollMagnitude(def, level, rng);
    }

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
    uint16_t inflicted = presetStatus;
    if (inflicted == 0) {
        bool failed = false;
        if (effectInflictedStatus(def) != 0 && (def->modeFlags & EffectModeRollResistance)) {
            failed = combatFailsSavingThrow((int16_t)level, (int16_t)threshold,
                                             (int16_t)effectResistanceBonus(def, recipientRecord), rng);
        }
        inflicted = effectResolveInflictedStatus(def, failed);
    }
    combatApplyEffect(recipientRecord, save, effectSpend(def), magnitude, material, inflicted);
}

static const Bcd4 g_zeroMaterial = {0, 0, 0, 0};

static void combatApplyTrapEffectToRecipient(uint8_t *recipientRecord, SaveGame *save, const EffectDef *def,
                                              unsigned threshold, RandomState *rng) {
    combatApplyEffectSlot(recipientRecord, save, def, 0, 0, g_zeroMaterial, threshold, rng);
}

static void combatApplyAttackerActionTo(uint8_t *defender, SaveGame *save, const ItemCatalog *catalog, GameKind game,
                                         const uint8_t *monster, const CombatEffectSelection *selection,
                                         const CombatAttackerAction *action, RandomState *rng) {
    EffectDef def;
    if (action->outcome == CombatAttackCorrosion) {
        combatApplyCorrosion(defender, catalog, game, selection, action);
        return;
    }
    if (!effectGetDef(game, selection->effectId, &def)) {
        return;
    }
    unsigned threshold = monsterGetU16(monster, MonsterFieldSaveDifficulty);
    if (action->outcome == CombatAttackDamage) {
        combatApplyEffectSlot(defender, save, &def, action->damage, 0, g_zeroMaterial, threshold, rng);
    } else {
        /* the slot's +0x10/+0x12 words are the gold-theft amount; its low word is also the preset "amount" */
        uint16_t low = (uint16_t)(action->goldAmount[0] | (action->goldAmount[1] << 8));
        combatApplyEffectSlot(defender, save, &def, low, 0, action->goldAmount, threshold, rng);
    }
}

CombatMonsterTurnOutcome combatProcessMonsterTurn(uint8_t *monster, uint8_t *singleTarget, SaveGame *save,
                                                    const ItemCatalog *catalog, GameKind game, RandomState *rng) {
    CombatMonsterTurnOutcome outcome;
    memset(&outcome, 0, sizeof(outcome));

    outcome.tick = monsterTickTimer(monster);
    if (monsterGetU16(monster, MonsterFieldState) & 0xF010) {
        monsterSetU16(monster, MonsterFieldState, (uint16_t)(monsterGetU16(monster, MonsterFieldState) | MonsterStateInfoRevealed));
    }
    if (outcome.tick != MonsterTickIdle) {
        return outcome;
    }

    if (monsterGetU16(monster, MonsterFieldFlags) & MonsterFlagAreaAttack) {
        CombatEffectSelection selection = combatSelectTrapEffectVariant(monster, rng);
        CombatAttackerAction actions[SavePartyMemberSlots];
        uint8_t *records[SavePartyMemberSlots];
        unsigned count = 0;
        for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
            uint16_t id = saveGetPartySlot(save, slot);
            if (id == 0) {
                break;
            }
            uint8_t *record = saveGamePartyRecordById(save, id);
            if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
                continue;
            }
            CombatAttackerAction action =
                combatResolveAttackerAction(monster, record, catalog, selection.isSpecial, rng);
            if (action.outcome != CombatAttackMiss) {
                actions[count] = action;
                records[count] = record;
                count++;
            }
        }
        /* The effect slots are applied together afterwards, in party order -- after every attack roll has been made. */
        for (unsigned i = 0; i < count; i++) {
            combatApplyAttackerActionTo(records[i], save, catalog, game, monster, &selection, &actions[i], rng);
        }
        outcome.attacked = count;
    } else if (singleTarget && !(partyGetU16(singleTarget, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
        CombatEffectSelection selection = combatSelectTrapEffectVariant(monster, rng);
        CombatAttackerAction action =
            combatResolveAttackerAction(monster, singleTarget, catalog, selection.isSpecial, rng);
        if (action.outcome != CombatAttackMiss) {
            combatApplyAttackerActionTo(singleTarget, save, catalog, game, monster, &selection, &action, rng);
            outcome.attacked = 1;
            if (!selection.isSpecial) {
                partyTickEquippedItemDurability(singleTarget, catalog, game, 0x146, rng);
            }
        }
    }
    outcome.idle = outcome.attacked == 0;

    /* the animation-state bits ProcessMonsterAttackTurn sets for the turn are cleared again at its end */
    monsterSetU16(monster, MonsterFieldState, (uint16_t)(monsterGetU16(monster, MonsterFieldState) & (uint16_t)~4u));
    monsterSetU16(monster, MonsterFieldAnim, monsterGetU16(monster, MonsterFieldSpriteBase));
    return outcome;
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

CombatLifeForceOutcome combatApplyLifeForceSpell(uint8_t *monsterRecord, uint8_t *casterRecord,
                                                   const uint8_t *spellRecord, SaveGame *save, GameKind game,
                                                   unsigned savingThrowThreshold, RandomState *rng) {
    CombatLifeForceOutcome outcome;
    memset(&outcome, 0, sizeof(outcome));

    /* ResolveAttackAndLatchFirstHit: a missed roll falls back to the record's own magnitude instead of nothing. */
    uint16_t damage = combatResolveAttack(monsterGetU16(monsterRecord, MonsterFieldAbsorption),
                                           partyGetStat(casterRecord, PartyStatCasting),
                                           spellGetU16(spellRecord, SpellFieldAttackMagnitude), rng);
    if (damage == 0) {
        outcome.fallback = true;
        damage = spellGetU16(spellRecord, SpellFieldAttackMagnitude);
    }
    outcome.damage = damage;

    uint16_t health = monsterGetU16(monsterRecord, MonsterFieldHealth);
    if (outcome.fallback) {
        monsterSetU16(monsterRecord, MonsterFieldHealth, (uint16_t)(health + damage));
        outcome.effectId = spellGetU16(spellRecord, SpellFieldLifeForceFallbackEffectId);
    } else {
        monsterSetU16(monsterRecord, MonsterFieldHealth, (uint16_t)(health - damage));
        monsterSetU16(monsterRecord, MonsterFieldLastAttackMarker, spellGetU16(spellRecord, SpellFieldInflictedMagnitude));
        monsterSetU16(monsterRecord, MonsterFieldState,
                      (uint16_t)(monsterGetU16(monsterRecord, MonsterFieldState) | MonsterStateHitFlashPending));
        outcome.effectId = spellGetU16(spellRecord, SpellFieldLifeForceHitEffectId);
    }

    EffectDef def;
    if (!effectGetDef(game, outcome.effectId, &def)) {
        return outcome;
    }
    uint16_t hpCost = spellGetU16(spellRecord, SpellFieldLifeForceHpCost);

    if (spellGetU16(spellRecord, SpellFieldResistFlags) & SpellResistLifeForceCaster) {
        /* The "status" word is the damage itself -- see combat.h. */
        combatApplyEffectSlot(casterRecord, save, &def, hpCost, damage, g_zeroMaterial, savingThrowThreshold, rng);
        outcome.recipients = 1;
        return outcome;
    }

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record) {
            continue;
        }
        if (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead) {
            combatApplyEffectSlot(record, save, &def, 0, 0, g_zeroMaterial, savingThrowThreshold, rng);
        } else {
            combatApplyEffectSlot(record, save, &def, hpCost, damage, g_zeroMaterial, savingThrowThreshold, rng);
        }
        outcome.recipients++;
    }
    return outcome;
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

uint16_t combatRollTrapAvoidanceMagnitude(GameKind game, uint16_t survivalStat, uint16_t threshold,
                                            uint16_t magnitudeCap, RandomState *rng) {
    int margin = (int)threshold - (int)survivalStat;
    if (margin < 0) {
        return 0;
    }
    /* Chapter 2 rolls out of 100; Chapter 3 rolls out of 55 instead (a real, easy-to-miss difference --
       the final magnitude formula below still divides by 100 unchanged in both games, only the roll's own
       upper bound differs, making Chapter 3 traps meaningfully more likely to trigger for the same margin). */
    int roll = randomInRange(rng, game == GameYendor2 ? 100 : 55);
    if (margin < roll) {
        return 0;
    }
    return (uint16_t)((magnitudeCap * (unsigned)margin + 50) / 100);
}

CombatSideTrapOutcome combatResolveSideTrap(GameKind game, uint16_t partyFacing, uint16_t monsterWoundFlags,
                                              uint16_t survivalStat, uint16_t threshold, uint16_t magnitudeCap,
                                              RandomState *rng) {
    CombatSideTrapOutcome result;
    result.magnitude = combatRollTrapAvoidanceMagnitude(game, survivalStat, threshold, magnitudeCap, rng);

    uint16_t requiredBit;
    switch (partyFacing) {
        case SaveFacingNorth:
            requiredBit = MonsterWoundPartyMustFaceNorth;
            break;
        case SaveFacingSouth:
            requiredBit = MonsterWoundPartyMustFaceSouth;
            break;
        case SaveFacingEast:
            requiredBit = MonsterWoundPartyMustFaceEast;
            break;
        default: /* SaveFacingWest, and matches the original's own unconditional else */
            requiredBit = MonsterWoundPartyMustFaceWest;
            break;
    }
    result.facingReady = (monsterWoundFlags & requiredBit) != 0;
    return result;
}

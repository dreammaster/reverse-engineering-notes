/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_combat test_combat.c ../combat.c ../effect.c ../monsterpool.c ../dungeongrid.c ../movement.c ../party.c ../monster.c ../monster_stdio.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c ../bcd4.c ../globalflags.c ../item.c ../spellrecord.c && ./test_combat
 */
#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "party.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static uint8_t g_monsterSlots[CombatMonsterSlotCount * MonsterRecordSize];

static void setMonsterSlot(unsigned slot, uint16_t typeId, uint16_t dexterity) {
    uint8_t *record = g_monsterSlots + (size_t)slot * MonsterRecordSize;
    memset(record, 0, MonsterRecordSize);
    monsterSetU16(record, MonsterFieldType, typeId);
    monsterSetU16(record, MonsterFieldDexterity, dexterity);
}

static void setupParty(SaveGame *save, uint16_t dex1, uint16_t dex2, uint16_t dex3, uint16_t dex4) {
    saveGameInit(save, GameYendor2);
    uint16_t dex[4] = {dex1, dex2, dex3, dex4};
    for (unsigned i = 0; i < 4; i++) {
        saveHeaderSetU16(save, SaveHeaderPartySlots + i * 2, (uint16_t)(i + 1));
        uint8_t *record = saveGamePartyRecordById(save, i + 1);
        partySetStat(record, PartyStatDexterity, dex[i]);
    }
}

static void testTurnOrderSortedDescending(void) {
    SaveGame save;
    setupParty(&save, 10, 30, 20, 5);
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 15);
    setMonsterSlot(1, 101, 40);
    setMonsterSlot(2, 0, 0); /* empty */

    RandomState rng;
    randomStart(&rng, 30, 50);
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    uint16_t targets[CombatMonsterSlotCount];
    unsigned count = combatBuildTurnOrder(&save, g_monsterSlots, &rng, order, targets);

    checkU32("6 entries: 4 party + 2 occupied monster slots", count, 6);
    checkU32("sorted descending: first is monster slot 1 (speed 40)", order[0].speed, 40);
    check("first entry is the monster", order[0].isMonster && order[0].index == 1);
    checkU32("second is party slot 1 (speed 30)", order[1].speed, 30);
    check("second entry is party", !order[1].isMonster && order[1].index == 1);
    checkU32("third is party slot 2 (speed 20)", order[2].speed, 20);
    checkU32("fourth is monster slot 0 (speed 15)", order[3].speed, 15);
    checkU32("fifth is party slot 0 (speed 10)", order[4].speed, 10);
    checkU32("sixth is party slot 3 (speed 5)", order[5].speed, 5);

    check("both occupied monster slots got a target", targets[0] != 0 && targets[1] != 0);
    checkU32("the empty monster slot has no target", targets[2], 0);
}

static void testStableTiesKeepBuildOrder(void) {
    SaveGame save;
    setupParty(&save, 20, 20, 20, 20);
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));

    RandomState rng;
    randomStart(&rng, 10, 20);
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    uint16_t targets[CombatMonsterSlotCount];
    unsigned count = combatBuildTurnOrder(&save, g_monsterSlots, &rng, order, targets);

    checkU32("4 tied party entries", count, 4);
    check("ties keep build order (party slots 0,1,2,3 in that order)",
          !order[0].isMonster && order[0].index == 0 && !order[1].isMonster && order[1].index == 1 &&
              !order[2].isMonster && order[2].index == 2 && !order[3].isMonster && order[3].index == 3);
}

static void testIncapacitatedPartyMembersExcluded(void) {
    SaveGame save;
    setupParty(&save, 10, 20, 30, 40);
    uint8_t *stunned = saveGamePartyRecordById(&save, 2);
    partySetU16(stunned, PartyFieldStatusFlags, PartyStatusStoned);
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));

    RandomState rng;
    randomStart(&rng, 5, 5);
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    uint16_t targets[CombatMonsterSlotCount];
    unsigned count = combatBuildTurnOrder(&save, g_monsterSlots, &rng, order, targets);

    checkU32("incapacitated party slot 1 (id 2) excluded: 3 entries", count, 3);
    for (unsigned i = 0; i < count; i++) {
        check("no entry is the incapacitated party slot", order[i].isMonster || order[i].index != 1);
    }
}

static void testNoLivingPartyLeavesMonsterUntargeted(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    /* All 4 party slots incapacitated. */
    for (unsigned i = 0; i < 4; i++) {
        saveHeaderSetU16(&save, SaveHeaderPartySlots + i * 2, (uint16_t)(i + 1));
        uint8_t *record = saveGamePartyRecordById(&save, i + 1);
        partySetU16(record, PartyFieldStatusFlags, PartyStatusDead);
    }
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 55, 12);

    RandomState rng;
    randomStart(&rng, 1, 1);
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    uint16_t targets[CombatMonsterSlotCount];
    unsigned count = combatBuildTurnOrder(&save, g_monsterSlots, &rng, order, targets);

    checkU32("only the one monster is in the turn order", count, 1);
    checkU32("no living party member: monster left untargeted rather than looping forever", targets[0], 0);
}

static void testSelectActiveMonster(void) {
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){false, 0, 30};
    order[1] = (CombatTurnOrderEntry){true, 1, 25};
    order[2] = (CombatTurnOrderEntry){true, 0, 15};

    bool defeated[CombatMonsterSlotCount] = {false, false, false};
    unsigned slot;
    check("first monster in turn order (slot 1) is active", combatSelectActiveMonster(order, 3, defeated, &slot) &&
                                                                  slot == 1);

    defeated[1] = true;
    check("defeated monster skipped, next one (slot 0) is active",
          combatSelectActiveMonster(order, 3, defeated, &slot) && slot == 0);

    defeated[0] = true;
    check("no monster active once both are defeated", !combatSelectActiveMonster(order, 3, defeated, &slot));
}

static uint32_t bcdHex(const uint8_t *value) {
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static void testProcessRoundAdvancesWithNoDeaths(void) {
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 10);
    monsterSetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldHealth, 5);

    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){false, 0, 30};
    order[1] = (CombatTurnOrderEntry){true, 0, 10};
    bool defeated[CombatMonsterSlotCount] = {false, false, false};
    unsigned cursor = 0;
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));

    CombatRoundOutcome outcome = combatProcessRound(g_monsterSlots, order, 2, defeated, &cursor, &staging, NULL, 0);

    check("still-alive monster: round continues", outcome == CombatRoundContinue);
    checkU32("cursor advances to entry 1", cursor, 1);
    check("no slot flagged defeated", !defeated[0] && !defeated[1] && !defeated[2]);
    checkU32("no rewards staged", bcdHex(staging.gold), 0);
}

static void testProcessRoundGrantsRewardsAndSkipsDefeated(void) {
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 10); /* dies this round */
    monsterSetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldHealth, 0);
    bcd4FromU16((uint8_t *)(g_monsterSlots + 0 * MonsterRecordSize + MonsterFieldLootGold), 50);
    setMonsterSlot(1, 101, 20); /* still alive, acts later this round */
    monsterSetU16(g_monsterSlots + 1 * MonsterRecordSize, MonsterFieldHealth, 40);

    /* Slot 0's own turn-order entry (index 0 here) just acted -- it's the one
     * that died -- so the scan for the next turn starts looking from entry 1
     * onward, matching the original's forward-only, no-wraparound walk. */
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){true, 0, 10};
    order[1] = (CombatTurnOrderEntry){true, 1, 20};
    bool defeated[CombatMonsterSlotCount] = {false, false, false};
    unsigned cursor = 0;
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));

    CombatRoundOutcome outcome = combatProcessRound(g_monsterSlots, order, 2, defeated, &cursor, &staging, NULL, 0);

    check("a monster is still alive: round continues", outcome == CombatRoundContinue);
    check("dead slot 0 flagged defeated", defeated[0]);
    check("live slot 1 not flagged defeated", !defeated[1]);
    checkU32("dead monster's gold staged", bcdHex(staging.gold), 0x50);
    checkU32("dead slot's record zeroed", monsterGetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldType), 0);
    checkU32("cursor advances past the dead entry to the still-alive one", cursor, 1);
}

static void testProcessRoundNoMonstersLeft(void) {
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 10);
    monsterSetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldHealth, 0);

    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){true, 0, 10};
    bool defeated[CombatMonsterSlotCount] = {false, false, false};
    unsigned cursor = 0;
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));

    CombatRoundOutcome outcome = combatProcessRound(g_monsterSlots, order, 1, defeated, &cursor, &staging, NULL, 0);

    check("last monster died: no monsters left", outcome == CombatRoundNoMonstersLeft);
    check("it's flagged defeated too", defeated[0]);
}

static void testProcessRoundEndOfListStartsNewRound(void) {
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 10);
    monsterSetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldHealth, 40);

    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){true, 0, 10};
    bool defeated[CombatMonsterSlotCount] = {false, false, false};
    unsigned cursor = 0; /* already on the last (only) entry */
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));

    CombatRoundOutcome outcome = combatProcessRound(g_monsterSlots, order, 1, defeated, &cursor, &staging, NULL, 0);

    check("walked off the end of the turn order: caller should rebuild it", outcome == CombatRoundNewRound);
}

static void testProcessRoundAdvanceSkipsAlreadyDefeatedEntries(void) {
    memset(g_monsterSlots, 0, sizeof(g_monsterSlots));
    setMonsterSlot(0, 100, 30);
    monsterSetU16(g_monsterSlots + 0 * MonsterRecordSize, MonsterFieldHealth, 40);
    setMonsterSlot(1, 101, 20);
    monsterSetU16(g_monsterSlots + 1 * MonsterRecordSize, MonsterFieldHealth, 40);

    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    order[0] = (CombatTurnOrderEntry){true, 0, 30};
    order[1] = (CombatTurnOrderEntry){true, 1, 20}; /* already defeated from an earlier round */
    order[2] = (CombatTurnOrderEntry){false, 0, 10};
    bool defeated[CombatMonsterSlotCount] = {false, true, false};
    unsigned cursor = 0;
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));

    CombatRoundOutcome outcome = combatProcessRound(g_monsterSlots, order, 3, defeated, &cursor, &staging, NULL, 0);

    check("round continues", outcome == CombatRoundContinue);
    checkU32("already-defeated entry 1 is skipped, lands on entry 2", cursor, 2);
}

static void testResolveAttackMissesOnZeroPower(void) {
    RandomState rng;
    randomStart(&rng, 1, 1);
    checkU32("zero power always misses", combatResolveAttack(10, 100, 0, &rng), 0);
}

static void testResolveAttackMissesWhenOutclassed(void) {
    RandomState rng;
    randomStart(&rng, 2, 2);
    checkU32("accuracy below defense always misses", combatResolveAttack(50, 10, 20, &rng), 0);
}

static void testResolveAttackMatchesRollForRollGatedOutcome(void) {
    /* diff == 1: the roll (0..55) only allows a hit when it comes up 0 or 1.
     * Peek the same generator's own roll on a copy to compute the expected
     * outcome directly, rather than looping until a seed happens to hit. */
    for (uint8_t seed = 0; seed < 10; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        uint16_t roll = randomInRange(&peek, 55);

        uint16_t damage = combatResolveAttack(9, 10, 3, &rng);
        if (roll <= 1) {
            /* hit: (3*1+50)/100 = 0, clamped to the minimum of 1 */
            checkU32("roll allows a hit: damage is the clamped minimum", damage, 1);
        } else {
            checkU32("roll exceeds the 1-point diff: miss", damage, 0);
        }
    }
}

static void testResolveAttackDamageFormula(void) {
    /* diff = 100 guarantees a hit for any roll in 0..55. */
    RandomState rng;
    randomStart(&rng, 3, 3);
    /* (20 * 100 + 50) / 100 = 20 */
    checkU32("damage = (power*diff+50)/100", combatResolveAttack(0, 100, 20, &rng), 20);
}

static void testFailsSavingThrowAlwaysResistsWhenChanceIsAtLeast100(void) {
    /* chance = 5*(50-0) + 0 = 250, clamped nowhere (already >= 5); randomInRange(100)
     * can never exceed 100, so the throw can never fail. */
    for (uint8_t seed = 0; seed < 10; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        check("chance far above the roll's max: saving throw always succeeds",
              !combatFailsSavingThrow(50, 0, 0, &rng));
    }
}

static void testFailsSavingThrowMatchesRollAgainstClampedFloor(void) {
    /* defenderStat == threshold, bonus == 0: chance = 5*0+0 = 0, clamped to the floor of 5. */
    for (uint8_t seed = 0; seed < 10; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        uint16_t roll = randomInRange(&peek, 100);

        bool fails = combatFailsSavingThrow(20, 20, 0, &rng);
        check("outcome matches roll against the clamped chance floor of 5", fails == (roll > 5));
    }
}

static void testApplyEffectHpCost(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatHitPoints, 30);

    combatApplyEffect(record, NULL, EffectSpendHp, 12, NULL, 0);

    checkU32("HP cost deducted", partyGetStat(record, PartyStatHitPoints), 18);
    checkU32("no status inflicted", partyGetU16(record, PartyFieldStatusFlags), 0);
}

static void testApplyEffectMpCost(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatMagicPoints, 10);

    combatApplyEffect(record, NULL, EffectSpendMp, 25, NULL, 0);

    checkU32("MP cost clamped at 0", partyGetStat(record, PartyStatMagicPoints), 0);
}

static void testApplyEffectHpAndMpCost(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatHitPoints, 30);
    partySetStat(record, PartyStatMagicPoints, 30);

    combatApplyEffect(record, NULL, EffectSpendHpAndMp, 5, NULL, 0);

    checkU32("HP+MP cost deducts the same amount from both", partyGetStat(record, PartyStatHitPoints), 25);
    checkU32("HP+MP cost deducts the same amount from both (MP)", partyGetStat(record, PartyStatMagicPoints), 25);
}

static void testApplyEffectInflictsStatus(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatHitPoints, 30);
    partySetU16(record, PartyFieldStatusFlags, PartyStatusCursed); /* a pre-existing, unrelated flag */

    combatApplyEffect(record, NULL, EffectSpendHp, 5, NULL, PartyStatusPoisoned);

    checkU32("HP cost still applied alongside a status", partyGetStat(record, PartyStatHitPoints), 25);
    check("the new status is OR'd in, not replacing existing flags",
          (partyGetU16(record, PartyFieldStatusFlags) & (PartyStatusCursed | PartyStatusPoisoned)) ==
              (PartyStatusCursed | PartyStatusPoisoned));
}

/* MonsterFieldGoldTheftAmount, effect id 15's own confirmed use: a monster's special attack stealing gold. */
static void testApplyEffectGoldTheft(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatHitPoints, 30);

    SaveGame save;
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderGold), 1000);

    Bcd4 stolen;
    bcd4FromU16(stolen, 16);
    combatApplyEffect(record, &save, EffectSpendGold, 0, stolen, 0);

    checkU32("gold stolen from the party's shared counter", bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x00000984);
    checkU32("HP untouched by a gold-cost effect", partyGetStat(record, PartyStatHitPoints), 30);
}

static void testApplyEffectOreCosts(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));

    SaveGame save;
    saveGameInit(&save, GameYendor2);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderOreCounter1), 10);
    bcd4FromU16(saveHeaderBcd4(&save, SaveHeaderOreCounter2), 10);

    Bcd4 amount;
    bcd4FromU16(amount, 20); /* more than the counter holds -- exercises the clamp */
    combatApplyEffect(record, &save, EffectSpendOre1, 0, amount, 0);
    combatApplyEffect(record, &save, EffectSpendOre2, 0, amount, 0);

    checkU32("ore1 clamped at 0 rather than underflowing", bcdHex(saveHeaderBcd4(&save, SaveHeaderOreCounter1)), 0);
    checkU32("ore2 clamped at 0 rather than underflowing", bcdHex(saveHeaderBcd4(&save, SaveHeaderOreCounter2)), 0);
}

static void testApplyEffectNoneIsNoOp(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetStat(record, PartyStatHitPoints, 30);
    partySetStat(record, PartyStatMagicPoints, 30);

    combatApplyEffect(record, NULL, EffectSpendNone, 999, NULL, 0);

    checkU32("EffectSpendNone touches nothing: HP", partyGetStat(record, PartyStatHitPoints), 30);
    checkU32("EffectSpendNone touches nothing: MP", partyGetStat(record, PartyStatMagicPoints), 30);
}

static uint8_t g_attacker[MonsterRecordSize];

static void setupAttacker(uint16_t attackEffect, uint16_t specialEffect, uint16_t state) {
    memset(g_attacker, 0, sizeof(g_attacker));
    monsterSetU16(g_attacker, MonsterFieldAttackEffect, attackEffect);
    monsterSetU16(g_attacker, MonsterFieldSpecialAttack, specialEffect);
    monsterSetU16(g_attacker, MonsterFieldState, state);
}

static void testSelectTrapEffectVariantDisabledOrNoSpecial(void) {
    RandomState rng;

    setupAttacker(10, 20, MonsterStateSpecialAttackDisabled);
    randomStart(&rng, 1, 1);
    CombatEffectSelection selection = combatSelectTrapEffectVariant(g_attacker, &rng);
    check("MonsterStateSpecialAttackDisabled: always the ordinary effect",
          selection.effectId == 10 && !selection.isSpecial);

    setupAttacker(10, 0, 0);
    randomStart(&rng, 2, 2);
    selection = combatSelectTrapEffectVariant(g_attacker, &rng);
    check("no special attack configured: always the ordinary effect", selection.effectId == 10 && !selection.isSpecial);
}

static void testSelectTrapEffectVariantRoll(void) {
    setupAttacker(10, 20, 0);
    for (uint8_t seed = 0; seed < 10; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        uint16_t roll = randomInRange(&peek, 100);

        CombatEffectSelection selection = combatSelectTrapEffectVariant(g_attacker, &rng);
        if (roll < 25) {
            check("roll < 25: special effect selected", selection.effectId == 20 && selection.isSpecial);
        } else {
            check("roll >= 25: ordinary effect selected", selection.effectId == 10 && !selection.isSpecial);
        }
    }
}

static void testResolveAttackerActionDamagePath(void) {
    uint8_t defender[PartyRecordSize];
    memset(defender, 0, sizeof(defender));

    setupAttacker(0, 0, 0);
    monsterSetU16(g_attacker, MonsterFieldAccuracy, 100);
    monsterSetU16(g_attacker, MonsterFieldDamage, 20);
    partySetStat(defender, PartyStatEquipRating5, 0);

    RandomState rng;
    randomStart(&rng, 5, 5);
    CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, NULL, false, &rng);
    check("isSpecial=false, guaranteed hit: CombatAttackDamage", action.outcome == CombatAttackDamage);
    checkU32("damage matches (power*diff+50)/100", action.damage, 20);

    monsterSetU16(g_attacker, MonsterFieldAccuracy, 0);
    partySetStat(defender, PartyStatEquipRating5, 9999);
    randomStart(&rng, 5, 5);
    action = combatResolveAttackerAction(g_attacker, defender, NULL, false, &rng);
    check("isSpecial=false, defense far exceeds accuracy: CombatAttackMiss", action.outcome == CombatAttackMiss);
}

static void testResolveAttackerActionGoldTheft(void) {
    uint8_t defender[PartyRecordSize];

    for (uint8_t seed = 0; seed < 10; seed++) {
        memset(defender, 0, sizeof(defender));
        partySetU16(defender, PartyFieldLevel, 10);
        partySetStat(defender, PartyStatSurvival, 5);

        setupAttacker(0, 15, 0); /* MonsterFlagSpecialMask left clear */
        monsterSetU16(g_attacker, MonsterFieldSaveDifficulty, 8);
        bcd4FromU16(g_attacker + MonsterFieldGoldTheftAmount, 16);

        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        bool failed = combatFailsSavingThrow(10, 8, (int16_t)5, &peek);

        CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, NULL, true, &rng);
        if (failed) {
            check("saving throw failed: CombatAttackStatusEffect", action.outcome == CombatAttackStatusEffect);
            checkU32("goldAmount matches the attacker's MonsterFieldGoldTheftAmount",
                     (uint32_t)action.goldAmount[0] << 24 | (uint32_t)action.goldAmount[1] << 16 |
                         (uint32_t)action.goldAmount[2] << 8 | action.goldAmount[3],
                     0x16);
        } else {
            check("saving throw resisted: CombatAttackMiss", action.outcome == CombatAttackMiss);
        }
    }
}

static void testResolveAttackerActionNoGoldAmountFallsBackToDamage(void) {
    uint8_t defender[PartyRecordSize];
    memset(defender, 0, sizeof(defender));
    partySetStat(defender, PartyStatEquipRating5, 0);

    setupAttacker(0, 15, 0);
    monsterSetU16(g_attacker, MonsterFieldAccuracy, 100);
    monsterSetU16(g_attacker, MonsterFieldDamage, 5);
    /* MonsterFieldGoldTheftAmount left at 0 (memset) and MonsterFlagSpecialMask clear:
     * a monster whose special effect was selected but has nothing configured for it. */

    RandomState rng;
    randomStart(&rng, 3, 3);
    CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, NULL, true, &rng);
    check("special selected but no gold amount and no corrode flags: falls back to a normal damage roll",
          action.outcome == CombatAttackDamage);
}

static ItemCatalog g_actionCatalog;

static void setupWeaponWithReplacement(uint16_t itemId, uint16_t replacementId) {
    memset(&g_actionCatalog, 0, sizeof(g_actionCatalog));
    g_actionCatalog.game = GameYendor2;
    g_actionCatalog.itemCount = itemId;
    g_actionCatalog.weaponCount = 1;
    uint8_t *item = g_actionCatalog.items + (size_t)(itemId - 1) * ItemRecordSize;
    item[ItemFieldFlags] = 0x00;
    item[ItemFieldFlags + 1] = 0x80; /* ItemFlagEquipCode0A -- category A */
    item[ItemFieldTargetOffset] = 0;
    item[ItemFieldTargetOffset + 1] = 0;
    uint8_t *weapon = g_actionCatalog.weapons;
    weapon[ItemTargetBreakItemA * 2] = (uint8_t)replacementId;
    weapon[ItemTargetBreakItemA * 2 + 1] = (uint8_t)(replacementId >> 8);
}

static void testResolveAttackerActionCorrosion(void) {
    uint8_t defender[PartyRecordSize];

    for (uint8_t seed = 0; seed < 10; seed++) {
        memset(defender, 0, sizeof(defender));
        partySetU16(defender, PartyFieldLevel, 10);
        partySetStat(defender, PartyStatSurvival, 10);
        itemSlotSet(partyEquipmentSlot(defender, 0x0A, GameYendor2), 5, 0); /* main weapon: item id 5 */
        setupWeaponWithReplacement(5, 999);

        setupAttacker(0, 15, 0);
        monsterSetU16(g_attacker, MonsterFieldFlags, MonsterFlagCorrodeWeaponSlot);
        monsterSetU16(g_attacker, MonsterFieldSaveDifficulty, 6);

        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        bool failed = combatFailsSavingThrow(10, 6, (int16_t)(10 / 2), &peek);

        CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, &g_actionCatalog, true, &rng);
        if (failed) {
            check("corrosion save failed: CombatAttackCorrosion", action.outcome == CombatAttackCorrosion);
            checkU32("targets the main weapon slot (0x13A)", action.equipSlotOffset, 0x13A);
            checkU32("finds the equipped item id", action.equippedItemId, 5);
            checkU32("replacement id matches the item's own ItemTargetBreakItemA", action.corrosionReplacementId, 999);
        } else {
            check("corrosion save resisted: CombatAttackMiss", action.outcome == CombatAttackMiss);
        }
    }
}

static void testResolveAttackerActionCorrosionSlotSelection(void) {
    uint8_t defender[PartyRecordSize];
    memset(defender, 0, sizeof(defender));
    partySetU16(defender, PartyFieldLevel, 1);
    partySetStat(defender, PartyStatSurvival, 0);

    setupAttacker(0, 15, 0);
    monsterSetU16(g_attacker, MonsterFieldFlags, MonsterFlagCorrodeSecondSlot);
    monsterSetU16(g_attacker, MonsterFieldSaveDifficulty, 0);

    RandomState rng;
    randomStart(&rng, 9, 9);
    CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, &g_actionCatalog, true, &rng);
    checkU32("MonsterFlagCorrodeSecondSlot targets 0x142", action.equipSlotOffset, 0x142);

    memset(defender, 0, sizeof(defender));
    partySetU16(defender, PartyFieldLevel, 1);
    partySetStat(defender, PartyStatSurvival, 0);
    monsterSetU16(g_attacker, MonsterFieldFlags, MonsterFlagSpecialCorrodeMask);
    randomStart(&rng, 9, 9);
    action = combatResolveAttackerAction(g_attacker, defender, &g_actionCatalog, true, &rng);
    checkU32("neither corrode-slot bit (just the special mask): targets 0x146", action.equipSlotOffset, 0x146);
}

static void testResolveAttackerActionCorrosionEmptySlotOrUnclassifiable(void) {
    uint8_t defender[PartyRecordSize];

    memset(defender, 0, sizeof(defender));
    partySetU16(defender, PartyFieldLevel, 1);
    partySetStat(defender, PartyStatSurvival, 0); /* very low chance to resist, but empty slot short-circuits first */
    setupAttacker(0, 15, 0);
    monsterSetU16(g_attacker, MonsterFieldFlags, MonsterFlagCorrodeWeaponSlot);
    monsterSetU16(g_attacker, MonsterFieldSaveDifficulty, 0);

    RandomState rng;
    randomStart(&rng, 1, 1);
    CombatAttackerAction action = combatResolveAttackerAction(g_attacker, defender, &g_actionCatalog, true, &rng);
    check("no item equipped at the targeted slot: CombatAttackMiss regardless of the save",
          action.outcome == CombatAttackMiss);

    /* An item that doesn't classify (no relevant flags) can't corrode even if equipped. */
    memset(&g_actionCatalog, 0, sizeof(g_actionCatalog));
    g_actionCatalog.game = GameYendor2;
    g_actionCatalog.itemCount = 5;
    itemSlotSet(partyEquipmentSlot(defender, 0x0A, GameYendor2), 5, 0);
    randomStart(&rng, 1, 1);
    action = combatResolveAttackerAction(g_attacker, defender, &g_actionCatalog, true, &rng);
    check("equipped item doesn't classify for item service: CombatAttackMiss", action.outcome == CombatAttackMiss);
}

static void testSavingThrowTrapNoneWhenPackedValueZero(void) {
    uint8_t acting[PartyRecordSize];
    memset(acting, 0, sizeof(acting));
    partySetStat(acting, PartyStatHitPoints, 999);

    RandomState rng;
    randomStart(&rng, 0, 0);
    CombatSavingThrowTrapOutcome outcome = combatApplySavingThrowTrap(0, acting, NULL, GameYendor2, &rng);

    check("packedValue == 0: CombatSavingThrowTrapNone, matching partyDecodeSavingThrowEffect's own false return",
          outcome == CombatSavingThrowTrapNone);
    checkU32("nothing applied", partyGetStat(acting, PartyStatHitPoints), 999);
}

static void testSavingThrowTrapAvoidedByHighSkill(void) {
    uint8_t acting[PartyRecordSize];
    memset(acting, 0, sizeof(acting));
    partySetU16(acting, PartyFieldLevel, 50);
    partySetStat(acting, PartyStatThievery, 0);
    partySetStat(acting, PartyStatHitPoints, 999);
    /* threshold=0, effectId=4 -> packedValue=4; chance = max(5, 5*(50-0)+0) = 250,
     * far above any possible randomInRange(100) roll -- the trigger can never fire. */

    for (uint8_t seed = 0; seed < 5; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        CombatSavingThrowTrapOutcome outcome = combatApplySavingThrowTrap(4, acting, NULL, GameYendor2, &rng);
        check("trigger chance saturates at a value randomInRange(100) can never exceed: always avoided",
              outcome == CombatSavingThrowTrapNone);
    }
    checkU32("nothing applied", partyGetStat(acting, PartyStatHitPoints), 999);
}

/* Effect id 4 (both games): HP cost, no fixed/scaled magnitude, no inflicted status --
 * exercises the magnitude roll without a resistance roll muddying the RNG sequence. */
static void testSavingThrowTrapSingleTargetAppliesEffect(void) {
    bool exercisedTrigger = false;
    for (uint8_t seed = 0; seed < 15; seed++) {
        uint8_t acting[PartyRecordSize];
        memset(acting, 0, sizeof(acting));
        partySetU16(acting, PartyFieldLevel, 10);
        partySetStat(acting, PartyStatThievery, 5);
        partySetStat(acting, PartyStatHitPoints, 999);

        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        bool triggerFails = combatFailsSavingThrow(10, 8, 5, &peek);

        /* packedValue = threshold(8)*100 + effectId(4) = 804; wholeParty is false (4 < 50). */
        CombatSavingThrowTrapOutcome outcome = combatApplySavingThrowTrap(804, acting, NULL, GameYendor2, &rng);

        if (!triggerFails) {
            check("trigger roll resisted: CombatSavingThrowTrapNone", outcome == CombatSavingThrowTrapNone);
            checkU32("HP untouched when the trap doesn't trigger", partyGetStat(acting, PartyStatHitPoints), 999);
            continue;
        }

        exercisedTrigger = true;
        check("trigger roll failed: CombatSavingThrowTrapSingle", outcome == CombatSavingThrowTrapSingle);

        EffectDef def;
        effectGetDef(GameYendor2, 4, &def);
        uint16_t randomValue = randomInRange(&peek, effectRandomBound(&def));
        uint16_t expectedMagnitude = effectMagnitude(&def, 10, randomValue);
        checkU32("HP reduced by exactly the rolled magnitude", partyGetStat(acting, PartyStatHitPoints),
                 (uint32_t)(999 - expectedMagnitude));
    }
    check("found at least one seed where the trigger roll fires", exercisedTrigger);
}

/* effectId 54 (54 - 50 = 4, the same simple HP-cost effect above) applied whole-party:
 * checks the incapacitated-skip and the stop-dead-at-the-first-empty-slot quirk together. */
static void testSavingThrowTrapWholeParty(void) {
    bool exercisedTrigger = false;
    for (uint8_t seed = 0; seed < 15; seed++) {
        SaveGame save;
        saveGameInit(&save, GameYendor2);

        uint8_t acting[PartyRecordSize];
        memset(acting, 0, sizeof(acting));
        /* The acting/triggering character's record is passed directly and is
         * deliberately not one of save's own party slots -- the whole-party
         * scan below walks SaveHeaderPartySlots independently of it. */

        saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
        uint8_t *slot0 = saveGamePartyRecordById(&save, 1);
        partySetStat(slot0, PartyStatHitPoints, 999);
        partySetU16(slot0, PartyFieldStatusFlags, PartyStatusStoned); /* incapacitated: skipped, loop continues */

        saveHeaderSetU16(&save, SaveHeaderPartySlots + 1 * 2, 2);
        uint8_t *slot1 = saveGamePartyRecordById(&save, 2);
        partySetU16(slot1, PartyFieldLevel, 10);
        partySetStat(slot1, PartyStatHitPoints, 999);

        saveHeaderSetU16(&save, SaveHeaderPartySlots + 2 * 2, 0); /* unoccupied: the scan stops dead here */

        saveHeaderSetU16(&save, SaveHeaderPartySlots + 3 * 2, 3);
        uint8_t *slot3 = saveGamePartyRecordById(&save, 3);
        partySetStat(slot3, PartyStatHitPoints, 999); /* never reached, thanks to the quirk above */

        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        bool triggerFails = combatFailsSavingThrow(0, 100, 0, &peek);

        /* packedValue = threshold(100)*100 + effectId(54) = 10054 -> threshold=100, effectId=4, wholeParty=true. */
        CombatSavingThrowTrapOutcome outcome = combatApplySavingThrowTrap(10054, acting, &save, GameYendor2, &rng);

        if (!triggerFails) {
            check("trigger roll resisted: CombatSavingThrowTrapNone", outcome == CombatSavingThrowTrapNone);
            checkU32("no slot touched when the trap doesn't trigger", partyGetStat(slot1, PartyStatHitPoints), 999);
            continue;
        }

        exercisedTrigger = true;
        check("trigger roll failed: CombatSavingThrowTrapParty", outcome == CombatSavingThrowTrapParty);
        checkU32("incapacitated slot 0 skipped, untouched", partyGetStat(slot0, PartyStatHitPoints), 999);
        check("occupied slot 1 got the effect", partyGetStat(slot1, PartyStatHitPoints) < 999);
        checkU32("slot 3 never reached: the scan stops dead at slot 2's empty id",
                 partyGetStat(slot3, PartyStatHitPoints), 999);
    }
    check("found at least one seed where the trigger roll fires", exercisedTrigger);
}

static void testSavingThrowTrapInvalidEffectIdIsNoEffect(void) {
    bool exercisedTrigger = false;
    for (uint8_t seed = 0; seed < 20; seed++) {
        uint8_t acting[PartyRecordSize];
        memset(acting, 0, sizeof(acting));

        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        bool triggerFails = combatFailsSavingThrow(0, 100, 0, &peek);
        if (!triggerFails) {
            continue;
        }
        exercisedTrigger = true;

        /* packedValue = threshold(100)*100 + effectId(99) = 10099; 99 is past EffectCountYendor2 (45). */
        CombatSavingThrowTrapOutcome outcome = combatApplySavingThrowTrap(10099, acting, NULL, GameYendor2, &rng);
        check("out-of-range effect id treated as no effect rather than reading past the table",
              outcome == CombatSavingThrowTrapNone);
    }
    check("found at least one seed where the trigger roll fires", exercisedTrigger);
}

static void testResolveEncodedItemEffectValue(void) {
    uint8_t acting[PartyRecordSize];
    memset(acting, 0, sizeof(acting));
    partySetU16(acting, PartyFieldStatusFlags, PartyStatusCursed);

    CombatEncodedItemEffectValue value = combatResolveEncodedItemEffectValue(true, acting, 0x4000, 7);
    checkU32("curse gate active + acting record cursed: inflictedStatus zeroed", value.inflictedStatus, 0);
    checkU32("curse gate active + acting record cursed: magnitude zeroed", value.magnitude, 0);

    memset(acting, 0, sizeof(acting)); /* not cursed */
    value = combatResolveEncodedItemEffectValue(true, acting, 0x4000, 7);
    checkU32("curse gate active but not cursed: value passes through (status)", value.inflictedStatus, 0x4000);
    checkU32("curse gate active but not cursed: value passes through (magnitude)", value.magnitude, 7);

    partySetU16(acting, PartyFieldStatusFlags, PartyStatusCursed);
    value = combatResolveEncodedItemEffectValue(false, acting, 0x4000, 7);
    checkU32("curse gate inactive: value passes through even if cursed (status)", value.inflictedStatus, 0x4000);
    checkU32("curse gate inactive: value passes through even if cursed (magnitude)", value.magnitude, 7);
}

/* Effect id 4 (both games): HP cost -- see testSavingThrowTrapSingleTargetAppliesEffect's own note. */
static void testApplyEncodedItemEffectSingleTargetsActingRecord(void) {
    uint8_t acting[PartyRecordSize];
    memset(acting, 0, sizeof(acting));
    partySetStat(acting, PartyStatHitPoints, 30);

    CombatEncodedItemEffectValue value = {0, 7};
    combatApplyEncodedItemEffectSingle(acting, NULL, 4, GameYendor2, value);

    checkU32("single-target HP cost applied to the acting record", partyGetStat(acting, PartyStatHitPoints), 23);
}

static void testApplyEncodedItemEffectSingleInvalidEffectIdIsNoOp(void) {
    uint8_t acting[PartyRecordSize];
    memset(acting, 0, sizeof(acting));
    partySetStat(acting, PartyStatHitPoints, 30);

    CombatEncodedItemEffectValue value = {0, 7};
    combatApplyEncodedItemEffectSingle(acting, NULL, 200, GameYendor2, value);

    checkU32("out-of-range effect id: no-op", partyGetStat(acting, PartyStatHitPoints), 30);
}

static void testApplyEncodedItemEffectPartyStopsAtEmptySlot(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
    uint8_t *slot0 = saveGamePartyRecordById(&save, 1);
    partySetStat(slot0, PartyStatHitPoints, 999);

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 1 * 2, 0); /* unoccupied: stops dead here */

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 2 * 2, 2);
    uint8_t *slot2 = saveGamePartyRecordById(&save, 2);
    partySetStat(slot2, PartyStatHitPoints, 999);

    CombatEncodedItemEffectValue value = {0, 7};
    combatApplyEncodedItemEffectParty(&save, 4, GameYendor2, value);

    checkU32("slot 0 got the effect", partyGetStat(slot0, PartyStatHitPoints), 992);
    checkU32("slot 2 never reached: the scan stops dead at slot 1's empty id",
             partyGetStat(slot2, PartyStatHitPoints), 999);
}

static void testApplyEncodedItemEffectPartyChapter2DoesNotSkipCursed(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
    uint8_t *slot0 = saveGamePartyRecordById(&save, 1);
    partySetStat(slot0, PartyStatHitPoints, 999);
    partySetU16(slot0, PartyFieldStatusFlags, PartyStatusCursed);

    CombatEncodedItemEffectValue value = {0, 7};
    combatApplyEncodedItemEffectParty(&save, 4, GameYendor2, value);

    checkU32("Chapter 2: a cursed party member still gets the whole-party effect",
             partyGetStat(slot0, PartyStatHitPoints), 992);
}

static void testApplyEncodedItemEffectPartyChapter3SkipsCursed(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor3);

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
    uint8_t *slot0 = saveGamePartyRecordById(&save, 1);
    partySetStat(slot0, PartyStatHitPoints, 999);
    partySetU16(slot0, PartyFieldStatusFlags, PartyStatusCursed);

    saveHeaderSetU16(&save, SaveHeaderPartySlots + 1 * 2, 2);
    uint8_t *slot1 = saveGamePartyRecordById(&save, 2);
    partySetStat(slot1, PartyStatHitPoints, 999);

    CombatEncodedItemEffectValue value = {0, 7};
    combatApplyEncodedItemEffectParty(&save, 4, GameYendor3, value);

    checkU32("Chapter 3: a cursed party member is skipped entirely", partyGetStat(slot0, PartyStatHitPoints), 999);
    checkU32("Chapter 3: the next (uncursed) member still gets the effect",
             partyGetStat(slot1, PartyStatHitPoints), 992);
}

/* effect id 22 (both games): one of the two "item replace" expiry-sound placeholder
 * effects (modeFlags EffectModeItemReplace, 0x400) -- the exact mode bit
 * ApplyEffectAndDrawIconBar's real dispatch keys off of to route a corrosion outcome
 * into HandleIconBarItemExpiry's replace branch (see combat.h's own doc comment). */
static void testApplyCorrosion(void) {
    ItemCatalog emptyCatalog;
    memset(&emptyCatalog, 0, sizeof(emptyCatalog));

    uint8_t defender[PartyRecordSize];
    memset(defender, 0, sizeof(defender));
    itemSlotSet(partyEquipmentSlot(defender, 0x0A, GameYendor2), 5, 0); /* main weapon: item id 5 */

    CombatEffectSelection selection = {22, true};
    CombatAttackerAction action;
    memset(&action, 0, sizeof(action));
    action.outcome = CombatAttackCorrosion;
    action.equipSlotOffset = 0x13A;
    action.equippedItemId = 5;
    action.corrosionReplacementId = 999;

    combatApplyCorrosion(defender, &emptyCatalog, GameYendor2, &selection, &action);

    checkU32("corrosion write-back replaces the equipped item",
             itemSlotId(partyEquipmentSlot(defender, 0x0A, GameYendor2)), 999);
    checkU32("...and parks the corroded item's own id as the slot's extra field",
             itemSlotExtra(partyEquipmentSlot(defender, 0x0A, GameYendor2)), 5);

    memset(defender, 0, sizeof(defender));
    itemSlotSet(partyEquipmentSlot(defender, 0x0A, GameYendor2), 5, 0);
    action.outcome = CombatAttackMiss;
    combatApplyCorrosion(defender, &emptyCatalog, GameYendor2, &selection, &action);
    checkU32("non-corrosion outcome: no-op, slot untouched",
             itemSlotId(partyEquipmentSlot(defender, 0x0A, GameYendor2)), 5);

    memset(defender, 0, sizeof(defender));
    itemSlotSet(partyEquipmentSlot(defender, 0x0A, GameYendor2), 5, 0);
    action.outcome = CombatAttackCorrosion;
    CombatEffectSelection badSelection = {9999, true}; /* out-of-range effect id */
    combatApplyCorrosion(defender, &emptyCatalog, GameYendor2, &badSelection, &action);
    checkU32("out-of-range effect id: no-op, slot untouched",
             itemSlotId(partyEquipmentSlot(defender, 0x0A, GameYendor2)), 5);
}

static void testApplyTargetResistances(void) {
    uint8_t target[MonsterRecordSize];

    /* Status filtering: an immune bit is dropped, a non-immune bit survives. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldImmunities, 0x8000); /* immune to Poison */
    CombatTargetAttackResult result = combatApplyTargetResistances(target, 10, 0x8000 | 0x4000, 0, 0);
    checkU32("poison filtered out (target is immune)", result.statusFlags, 0x4000);
    checkU32("damage untouched by status filtering", result.damage, 10);

    /* Full negation: a matching low bit zeroes damage outright, even alongside a surviving status bit. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldImmunities, 0x0002); /* immune to Electric */
    result = combatApplyTargetResistances(target, 10, 0x4000 | 0x0002, 0, 0);
    checkU32("a low-bit immunity match zeroes damage entirely", result.damage, 0);
    checkU32("...but the already-computed status flag from the high-bit pass survives",
             result.statusFlags, 0x4000);

    /* No immunity match at all: damage and (empty) status flags pass through untouched. */
    memset(target, 0, sizeof(target));
    result = combatApplyTargetResistances(target, 10, 0x0002, 0, 0);
    checkU32("no immunity match: damage untouched", result.damage, 10);
    checkU32("no immunity match: no status inflicted", result.statusFlags, 0);

    /* Resistance halving: first matching bit halves and returns immediately. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldResistances, 0x8000);
    result = combatApplyTargetResistances(target, 11, 0, 0x8000, 999);
    checkU32("matching resistance halves damage", result.damage, 5);

    /* Bit 0x200 requested but not matched: returns without halving or draining. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldDamage, 50);
    result = combatApplyTargetResistances(target, 10, 0x0020 /* would select MonsterFieldDamage if reached */,
                                           0x0200, 5);
    checkU32("bit 0x200 requested, not matched: damage untouched (no halving)", result.damage, 10);
    checkU32("...and the drain never runs either", monsterGetU16(target, MonsterFieldDamage), 50);

    /* Drain effect: only reached when resistanceFlags doesn't request bit 0x200 at all. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldDamage, 50);
    result = combatApplyTargetResistances(target, 10, 0x0020, 0, 5);
    checkU32("drain: bit 0x20 selects MonsterFieldDamage", monsterGetU16(target, MonsterFieldDamage), 45);
    checkU32("drain doesn't touch the returned damage value", result.damage, 10);

    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldHealth, 50);
    result = combatApplyTargetResistances(target, 10, 0x0200, 0, 999);
    checkU32("drain: bit 0x200 selects MonsterFieldHealth (not MaxHealth)",
             monsterGetU16(target, MonsterFieldHealth), 0);

    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldAccuracy, 30);
    result = combatApplyTargetResistances(target, 10, 0x0100, 0, 999);
    checkU32("drain: bit 0x100 selects MonsterFieldAccuracy, floored at 0",
             monsterGetU16(target, MonsterFieldAccuracy), 0);

    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldDexterity, 30);
    result = combatApplyTargetResistances(target, 10, 0x0080, 0, 12);
    checkU32("drain: bit 0x80 selects MonsterFieldDexterity", monsterGetU16(target, MonsterFieldDexterity), 18);

    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldAbsorption, 30);
    result = combatApplyTargetResistances(target, 10, 0x0040, 0, 12);
    checkU32("drain: bit 0x40 selects MonsterFieldAbsorption", monsterGetU16(target, MonsterFieldAbsorption), 18);

    /* Priority: bit 0x200 wins over the others when several drain bits are set at once. */
    memset(target, 0, sizeof(target));
    monsterSetU16(target, MonsterFieldHealth, 40);
    monsterSetU16(target, MonsterFieldDamage, 40);
    result = combatApplyTargetResistances(target, 10, 0x0200 | 0x0020, 0, 5);
    checkU32("drain priority: 0x200 (Health) wins over 0x20 (Damage)", monsterGetU16(target, MonsterFieldHealth), 35);
    checkU32("...the lower-priority field is untouched", monsterGetU16(target, MonsterFieldDamage), 40);
}

static void setSpellU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)(value & 0xFF);
    record[offset + 1] = (uint8_t)(value >> 8);
}

static void testResolveSpellAttackTypeRestrictionBlocksMismatch(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldUnknown4E, 5);
    setSpellU16(spell, SpellFieldResistFlags, SpellResistTypeRestricted);
    setSpellU16(spell, SpellFieldTargetTypeId, 9);
    setSpellU16(spell, SpellFieldAttackMagnitude, 50);

    randomStart(&rng, 1, 1);
    CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, true, &rng);
    check("a type-restricted attack against a mismatched target has no effect at all", !result.hasEffect);
    checkU32("...damage is 0", result.damage, 0);
}

static void testResolveSpellAttackTypeRestrictionAllowsMatch(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldUnknown4E, 9);
    setSpellU16(spell, SpellFieldResistFlags, SpellResistTypeRestricted);
    setSpellU16(spell, SpellFieldTargetTypeId, 9);
    setSpellU16(spell, SpellFieldAttackMagnitude, 50);

    randomStart(&rng, 1, 1);
    CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, true, &rng);
    check("a matching restricted type proceeds normally", result.hasEffect);
    checkU32("...using the preset magnitude directly", result.damage, 50);
}

static void testResolveSpellAttackAlreadyResolvedSkipsTheRoll(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldAttackMagnitude, 30);

    /* A power-0 roll would always miss, but alreadyResolved bypasses combatResolveAttack entirely. */
    randomStart(&rng, 1, 1);
    CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, true, &rng);
    check("already-resolved attacks don't roll at all", result.hasEffect);
    checkU32("...damage is the record's own magnitude verbatim", result.damage, 30);
}

static void testResolveSpellAttackNormalRollMissSkipsResistancesEntirely(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    /* power 0 (SpellFieldAttackMagnitude left at 0) always misses in combatResolveAttack. */
    monsterSetU16(target, MonsterFieldDamage, 50);
    setSpellU16(spell, SpellFieldAttackFlags, 0x0020); /* would drain MonsterFieldDamage if resistances ran at all */
    setSpellU16(spell, SpellFieldDrainAmount, 5);

    randomStart(&rng, 1, 1);
    CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, false, &rng);
    check("a missed roll has no effect", !result.hasEffect);
    checkU32("...and resistances/drain are never even consulted on a miss",
             monsterGetU16(target, MonsterFieldDamage), 50);
}

static void testResolveSpellAttackNormalRollHitUsesCasterAccuracyAndTargetAbsorption(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldAbsorption, 0);
    partySetStat(caster, PartyStatCasting, 100);
    setSpellU16(spell, SpellFieldAttackMagnitude, 20);

    /* diff = 100 guarantees a hit for any roll in 0..55 (see testResolveAttackDamageFormula). */
    randomStart(&rng, 3, 3);
    CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, false, &rng);
    check("a guaranteed-hit roll produces an effect", result.hasEffect);
    checkU32("...with (power*diff+50)/100 damage", result.damage, 20);
}

static void testApplySpellAttackIsNoOpWithoutEffect(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldHealth, 100);

    CombatSpellAttackResult result = {0, 0, false};
    combatApplySpellAttack(target, spell, result);
    checkU32("no effect: health untouched", monsterGetU16(target, MonsterFieldHealth), 100);
    checkU32("no effect: state untouched", monsterGetU16(target, MonsterFieldState), 0);
}

static void testApplySpellAttackCommitsDamageAndWakesTarget(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldHealth, 100);

    CombatSpellAttackResult result = {15, 0, true};
    combatApplySpellAttack(target, spell, result);
    checkU32("damage is subtracted from health", monsterGetU16(target, MonsterFieldHealth), 85);
    check("the target becomes Aware", monsterGetU16(target, MonsterFieldState) & MonsterStateAware);
    check("...and gets a hit-flash cue", monsterGetU16(target, MonsterFieldState) & MonsterStateHitFlashPending);
}

static void testApplySpellAttackStatusArmsTickTimerRegardlessOfPersistBit(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldTickAmount, 7);
    setSpellU16(spell, SpellFieldTickCountdown, 3);
    /* SpellFlagsAPersistAffliction deliberately left clear. */

    CombatSpellAttackResult result = {0, 0x0400 /* Cursing */, true};
    combatApplySpellAttack(target, spell, result);
    checkU32("tick amount is armed even without the persist-affliction bit",
             monsterGetU16(target, MonsterFieldTickAmount), 7);
    checkU32("...so is tick countdown", monsterGetU16(target, MonsterFieldTickCountdown), 3);
    checkU32("without the persist bit, immunities are not marked", monsterGetU16(target, MonsterFieldImmunities), 0);
    check("surviving status bits are OR'd into state, disabling the special attack (shared bit with Cursing)",
          monsterGetU16(target, MonsterFieldState) & MonsterStateSpecialAttackDisabled);
}

static void testApplySpellAttackPersistBitAlsoMarksImmunitiesAfflicted(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldFlagsA, SpellFlagsAPersistAffliction);

    CombatSpellAttackResult result = {0, 0x0800 /* Hexing */, true};
    combatApplySpellAttack(target, spell, result);
    checkU32("the persist bit also marks the target as currently afflicted",
             monsterGetU16(target, MonsterFieldImmunities), 0x0800);
    check("hexing also marks the target busy (shared bit)", monsterGetU16(target, MonsterFieldState) & MonsterStateBusy);
}

static void testApplySpellAttackHalfTargetDamageOverridesEvenAZeroResult(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    monsterSetU16(target, MonsterFieldHealth, 100);
    monsterSetU16(target, MonsterFieldDamage, 11);
    setSpellU16(spell, SpellFieldResistFlags, SpellResistHalfTargetDamage);

    /* damage is 0 in the result, but a status survived, so hasEffect is still true -- the half-damage override applies. */
    CombatSpellAttackResult result = {0, 0x0001, true};
    combatApplySpellAttack(target, spell, result);
    checkU32("half the target's own damage overrides the result's damage value, even when that was 0",
             monsterGetU16(target, MonsterFieldHealth), 100 - 5);
}

static void testApplySpellAttackClearAwareBit(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldResistFlags, SpellResistClearAware);

    CombatSpellAttackResult result = {5, 0, true};
    combatApplySpellAttack(target, spell, result);
    check("MonsterStateAware is cleared again at the end when the record asks for it",
          !(monsterGetU16(target, MonsterFieldState) & MonsterStateAware));
    check("the hit-flash cue is unaffected", monsterGetU16(target, MonsterFieldState) & MonsterStateHitFlashPending);
}

static void testMarkSpellAttackHitWritesTheInflictedMagnitudeField(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t spell[SpellRecordSize];
    memset(target, 0, sizeof(target));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldInflictedMagnitude, 42);

    combatMarkSpellAttackHit(target, spell);
    checkU32("the marker field gets the spell record's own inflicted-magnitude value",
             monsterGetU16(target, MonsterFieldLastAttackMarker), 42);
}

static void testApplySpellAttackToActiveSlotsSkipsEmptyAndDeadSlots(void) {
    uint8_t slots[MonsterActiveSlots * MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(slots, 0, sizeof(slots));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldAttackMagnitude, 10);

    /* Slot 0: empty (MonsterFieldType == 0) -- skipped. */
    /* Slot 1: occupied but already dead (MonsterFieldHealth == 0) -- skipped. */
    monsterSetU16(slots + 1 * MonsterRecordSize, MonsterFieldType, 7);
    monsterSetU16(slots + 1 * MonsterRecordSize, MonsterFieldHealth, 0);
    /* Slot 2: occupied and alive -- attacked. */
    monsterSetU16(slots + 2 * MonsterRecordSize, MonsterFieldType, 9);
    monsterSetU16(slots + 2 * MonsterRecordSize, MonsterFieldHealth, 100);

    randomStart(&rng, 1, 1);
    CombatSpellAreaAttackOutcome outcome =
        combatApplySpellAttackToActiveSlots(slots, caster, spell, true /* alreadyResolved */, &rng);

    check("slot 0 (empty) is skipped", !outcome.hit[0]);
    check("slot 1 (dead) is skipped", !outcome.hit[1]);
    check("slot 2 (alive) is attacked", outcome.hit[2]);
    checkU32("the attacked slot takes the preset damage", monsterGetU16(slots + 2 * MonsterRecordSize, MonsterFieldHealth),
             90);
    checkU32("untouched slots are unaffected", monsterGetU16(slots + 1 * MonsterRecordSize, MonsterFieldHealth), 0);
}

static void testApplySpellAttackToActiveSlotsHitsEveryEligibleSlot(void) {
    uint8_t slots[MonsterActiveSlots * MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    RandomState rng;

    memset(slots, 0, sizeof(slots));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    setSpellU16(spell, SpellFieldAttackMagnitude, 5);
    setSpellU16(spell, SpellFieldInflictedMagnitude, 99);

    for (unsigned slot = 0; slot < MonsterActiveSlots; slot++) {
        monsterSetU16(slots + slot * MonsterRecordSize, MonsterFieldType, slot + 1);
        monsterSetU16(slots + slot * MonsterRecordSize, MonsterFieldHealth, 50);
    }

    randomStart(&rng, 1, 1);
    CombatSpellAreaAttackOutcome outcome = combatApplySpellAttackToActiveSlots(slots, caster, spell, true, &rng);

    for (unsigned slot = 0; slot < MonsterActiveSlots; slot++) {
        check("every eligible slot is attacked", outcome.hit[slot]);
        checkU32("...and takes the preset damage", monsterGetU16(slots + slot * MonsterRecordSize, MonsterFieldHealth), 45);
        checkU32("...and gets the hit marker written", monsterGetU16(slots + slot * MonsterRecordSize, MonsterFieldLastAttackMarker),
                 99);
    }
}

static void testSaveLocationBookmarkWritesAllSevenFields(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));

    CombatLocationBookmark bookmark = {123, 456, 7, 11, 22, 33, 5};
    combatSaveLocationBookmark(record, 0xF0, bookmark);

    CombatLocationBookmark out;
    check("a freshly-saved bookmark restores successfully",
          combatRestoreLocationBookmark(record, 0xF0, &out));
    checkU32("world X round-trips", (uint16_t)out.worldX, 123);
    checkU32("world Y round-trips", (uint16_t)out.worldY, 456);
    checkU32("facing round-trips", out.facing, 7);
    checkU32("renderA round-trips", out.renderA, 11);
    checkU32("renderB round-trips", out.renderB, 22);
    checkU32("renderC round-trips", out.renderC, 33);
    checkU32("renderDLow3 round-trips", out.renderDLow3, 5);
}

static void testRestoreLocationBookmarkFailsWhenNeverSaved(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));

    CombatLocationBookmark out;
    check("a never-saved bookmark (world X == 0) fails to restore",
          !combatRestoreLocationBookmark(record, 0xF0, &out));
}

static void testApplyDamageToMapMonsterSurvivesHit(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    MonsterRewardStaging staging;
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    memset(&staging, 0, sizeof(staging));
    monsterSetU16(target, MonsterFieldHealth, 100);
    setSpellU16(spell, SpellFieldAttackMagnitude, 15);

    CombatMapMonsterAttackOutcome outcome =
        combatApplyDamageToMapMonster(target, caster, spell, true, &staging, NULL, 0, NULL, &rng);

    check("the attack landed", outcome.attack.hasEffect);
    check("the monster survives a non-lethal hit", !outcome.monsterDied);
    checkU32("health is reduced by the attack", monsterGetU16(target, MonsterFieldHealth), 85);
}

static void testApplyDamageToMapMonsterGrantsRewardsAndRemovesOnDeath(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    MonsterRewardStaging staging;
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    memset(&staging, 0, sizeof(staging));
    monsterSetU16(target, MonsterFieldHealth, 10);
    monsterSetU16(target, MonsterFieldType, 7); /* occupied */
    setSpellU16(spell, SpellFieldAttackMagnitude, 15);

    CombatMapMonsterAttackOutcome outcome =
        combatApplyDamageToMapMonster(target, caster, spell, true, &staging, NULL, 0, NULL, &rng);

    check("a lethal hit reports the monster died", outcome.monsterDied);
    checkU32("the record is zeroed by monsterPoolRemove", monsterGetU16(target, MonsterFieldType), 0);
}

static void testApplyDamageToMapMonsterMissDoesNothing(void) {
    uint8_t target[MonsterRecordSize];
    uint8_t caster[PartyRecordSize];
    uint8_t spell[SpellRecordSize];
    MonsterRewardStaging staging;
    RandomState rng;

    memset(target, 0, sizeof(target));
    memset(caster, 0, sizeof(caster));
    memset(spell, 0, sizeof(spell));
    memset(&staging, 0, sizeof(staging));
    monsterSetU16(target, MonsterFieldHealth, 100);
    monsterSetU16(target, MonsterFieldType, 7);
    /* SpellFieldAttackMagnitude left at 0 -- power 0 always misses on the normal roll path. */

    randomStart(&rng, 1, 1);
    CombatMapMonsterAttackOutcome outcome =
        combatApplyDamageToMapMonster(target, caster, spell, false, &staging, NULL, 0, NULL, &rng);

    check("a miss has no effect", !outcome.attack.hasEffect);
    check("...and definitely doesn't count as a kill", !outcome.monsterDied);
    checkU32("health is untouched", monsterGetU16(target, MonsterFieldHealth), 100);
    checkU32("the record is untouched, not removed", monsterGetU16(target, MonsterFieldType), 7);
}

static void testRollTrapAvoidanceMagnitudeNegativeMarginAvoidsWithoutRolling(void) {
    RandomState rng;
    randomStart(&rng, 3, 7);
    RandomState before = rng;
    /* threshold (20) < survivalStat (25) -> margin is negative -> avoided, and no RandomInRange call at all
       (fidelity check: the original's own early-out skips the roll entirely, so rng must be untouched). */
    uint16_t magnitude = combatRollTrapAvoidanceMagnitude(GameYendor2, 25, 20, 999, &rng);
    checkU32("negative margin avoids the trap", magnitude, 0);
    check("...and consumes no RNG state at all", memcmp(&rng, &before, sizeof(rng)) == 0);
}

static void testRollTrapAvoidanceMagnitudeChapter2UsesBound100(void) {
    for (uint8_t seed = 0; seed < 12; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        uint16_t roll = randomInRange(&peek, 100);

        uint16_t magnitude = combatRollTrapAvoidanceMagnitude(GameYendor2, 10, 60, 40, &rng);
        uint16_t margin = 50; /* threshold(60) - stat(10) */
        uint16_t expected = (roll > margin) ? 0 : (uint16_t)((40u * margin + 50) / 100);
        checkU32("Chapter 2 magnitude matches RandomInRange(100)-based formula", magnitude, expected);
    }
}

static void testRollTrapAvoidanceMagnitudeChapter3UsesBound55(void) {
    for (uint8_t seed = 0; seed < 12; seed++) {
        RandomState rng;
        randomStart(&rng, seed, seed);
        RandomState peek = rng;
        uint16_t roll = randomInRange(&peek, 55); /* the real, easy-to-miss Chapter 3 difference */

        uint16_t magnitude = combatRollTrapAvoidanceMagnitude(GameYendor3, 10, 60, 40, &rng);
        uint16_t margin = 50;
        uint16_t expected = (roll > margin) ? 0 : (uint16_t)((40u * margin + 50) / 100);
        checkU32("Chapter 3 magnitude matches RandomInRange(55)-based formula", magnitude, expected);
    }
}

static void testRollTrapAvoidanceMagnitudeSameSeedCanDifferBetweenGames(void) {
    /* A margin of 55-99 always triggers in Chapter 3 (roll in [0,55) never exceeds it) but only
       sometimes in Chapter 2 (roll in [0,100) can exceed it) -- the concrete, observable consequence
       of the differing roll bound, not just a formula difference on paper. */
    bool sawTriggerOnlyInChapter3 = false;
    for (uint8_t seed = 0; seed < 40; seed++) {
        RandomState rng2;
        randomStart(&rng2, seed, seed);
        uint16_t m2 = combatRollTrapAvoidanceMagnitude(GameYendor2, 0, 70, 100, &rng2);

        RandomState rng3;
        randomStart(&rng3, seed, seed);
        uint16_t m3 = combatRollTrapAvoidanceMagnitude(GameYendor3, 0, 70, 100, &rng3);

        if (m2 == 0 && m3 != 0) {
            sawTriggerOnlyInChapter3 = true;
        }
        /* Chapter 3 should never be strictly less likely to trigger than Chapter 2 for the same margin/seed. */
        checkU32("Chapter 3 triggers whenever Chapter 2 does, for the same seed/margin", (m2 != 0 && m3 == 0) ? 1 : 0,
                 0);
    }
    check("at least one seed demonstrates Chapter 3 triggering where Chapter 2 doesn't", sawTriggerOnlyInChapter3);
}

static void testResolveSideTrapFacingMatchesEachDirection(void) {
    RandomState rng;
    randomStart(&rng, 0, 0);

    CombatSideTrapOutcome r;
    r = combatResolveSideTrap(GameYendor2, SaveFacingNorth, MonsterWoundPartyMustFaceNorth, 100, 0, 0, &rng);
    check("facing North matches a North-armed trap", r.facingReady);
    r = combatResolveSideTrap(GameYendor2, SaveFacingNorth, MonsterWoundPartyMustFaceSouth, 100, 0, 0, &rng);
    check("facing North does not match a South-armed trap", !r.facingReady);

    r = combatResolveSideTrap(GameYendor2, SaveFacingSouth, MonsterWoundPartyMustFaceSouth, 100, 0, 0, &rng);
    check("facing South matches a South-armed trap", r.facingReady);

    r = combatResolveSideTrap(GameYendor2, SaveFacingEast, MonsterWoundPartyMustFaceEast, 100, 0, 0, &rng);
    check("facing East matches an East-armed trap", r.facingReady);

    /* SaveFacingWest is the original's own unconditional "else" branch -- no explicit cmp against it. */
    r = combatResolveSideTrap(GameYendor2, SaveFacingWest, MonsterWoundPartyMustFaceWest, 100, 0, 0, &rng);
    check("facing West (the implicit else) matches a West-armed trap", r.facingReady);
    r = combatResolveSideTrap(GameYendor2, SaveFacingWest, MonsterWoundPartyMustFaceEast, 100, 0, 0, &rng);
    check("facing West does not match an East-armed trap", !r.facingReady);
}

static void testResolveSideTrapRollsRegardlessOfFacing(void) {
    /* The roll always happens, even when the facing check will fail -- fidelity with the original's own
       RNG-draw-count, which rolls unconditionally before the facing branch. */
    RandomState rng;
    randomStart(&rng, 5, 5);
    RandomState peekRng = rng;
    uint16_t expectedMagnitude = combatRollTrapAvoidanceMagnitude(GameYendor2, 10, 60, 40, &peekRng);

    CombatSideTrapOutcome r =
        combatResolveSideTrap(GameYendor2, SaveFacingNorth, MonsterWoundPartyMustFaceSouth, 10, 60, 40, &rng);
    checkU32("the roll still happens (and matches) even though facing won't match", r.magnitude, expectedMagnitude);
    check("...and facingReady correctly reports false", !r.facingReady);
}

int main(void) {
    testTurnOrderSortedDescending();
    testStableTiesKeepBuildOrder();
    testIncapacitatedPartyMembersExcluded();
    testNoLivingPartyLeavesMonsterUntargeted();
    testSelectActiveMonster();
    testProcessRoundAdvancesWithNoDeaths();
    testProcessRoundGrantsRewardsAndSkipsDefeated();
    testProcessRoundNoMonstersLeft();
    testProcessRoundEndOfListStartsNewRound();
    testProcessRoundAdvanceSkipsAlreadyDefeatedEntries();
    testResolveAttackMissesOnZeroPower();
    testResolveAttackMissesWhenOutclassed();
    testResolveAttackMatchesRollForRollGatedOutcome();
    testResolveAttackDamageFormula();
    testFailsSavingThrowAlwaysResistsWhenChanceIsAtLeast100();
    testFailsSavingThrowMatchesRollAgainstClampedFloor();
    testApplyEffectHpCost();
    testApplyEffectMpCost();
    testApplyEffectHpAndMpCost();
    testApplyEffectInflictsStatus();
    testApplyEffectGoldTheft();
    testApplyEffectOreCosts();
    testApplyEffectNoneIsNoOp();
    testSelectTrapEffectVariantDisabledOrNoSpecial();
    testSelectTrapEffectVariantRoll();
    testResolveAttackerActionDamagePath();
    testResolveAttackerActionGoldTheft();
    testResolveAttackerActionNoGoldAmountFallsBackToDamage();
    testResolveAttackerActionCorrosion();
    testResolveAttackerActionCorrosionSlotSelection();
    testResolveAttackerActionCorrosionEmptySlotOrUnclassifiable();
    testApplyCorrosion();
    testApplyTargetResistances();
    testResolveSpellAttackTypeRestrictionBlocksMismatch();
    testResolveSpellAttackTypeRestrictionAllowsMatch();
    testResolveSpellAttackAlreadyResolvedSkipsTheRoll();
    testResolveSpellAttackNormalRollMissSkipsResistancesEntirely();
    testResolveSpellAttackNormalRollHitUsesCasterAccuracyAndTargetAbsorption();
    testApplySpellAttackIsNoOpWithoutEffect();
    testApplySpellAttackCommitsDamageAndWakesTarget();
    testApplySpellAttackStatusArmsTickTimerRegardlessOfPersistBit();
    testApplySpellAttackPersistBitAlsoMarksImmunitiesAfflicted();
    testApplySpellAttackHalfTargetDamageOverridesEvenAZeroResult();
    testApplySpellAttackClearAwareBit();
    testMarkSpellAttackHitWritesTheInflictedMagnitudeField();
    testApplySpellAttackToActiveSlotsSkipsEmptyAndDeadSlots();
    testApplySpellAttackToActiveSlotsHitsEveryEligibleSlot();
    testSaveLocationBookmarkWritesAllSevenFields();
    testRestoreLocationBookmarkFailsWhenNeverSaved();
    testApplyDamageToMapMonsterSurvivesHit();
    testApplyDamageToMapMonsterGrantsRewardsAndRemovesOnDeath();
    testApplyDamageToMapMonsterMissDoesNothing();
    testSavingThrowTrapNoneWhenPackedValueZero();
    testSavingThrowTrapAvoidedByHighSkill();
    testSavingThrowTrapSingleTargetAppliesEffect();
    testSavingThrowTrapWholeParty();
    testSavingThrowTrapInvalidEffectIdIsNoEffect();
    testResolveEncodedItemEffectValue();
    testApplyEncodedItemEffectSingleTargetsActingRecord();
    testApplyEncodedItemEffectSingleInvalidEffectIdIsNoOp();
    testApplyEncodedItemEffectPartyStopsAtEmptySlot();
    testApplyEncodedItemEffectPartyChapter2DoesNotSkipCursed();
    testApplyEncodedItemEffectPartyChapter3SkipsCursed();
    testRollTrapAvoidanceMagnitudeNegativeMarginAvoidsWithoutRolling();
    testRollTrapAvoidanceMagnitudeChapter2UsesBound100();
    testRollTrapAvoidanceMagnitudeChapter3UsesBound55();
    testRollTrapAvoidanceMagnitudeSameSeedCanDifferBetweenGames();
    testResolveSideTrapFacingMatchesEachDirection();
    testResolveSideTrapRollsRegardlessOfFacing();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

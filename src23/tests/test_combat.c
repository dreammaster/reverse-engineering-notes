/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_combat test_combat.c ../combat.c ../effect.c ../monsterpool.c ../dungeongrid.c ../movement.c ../party.c ../monster.c ../monster_stdio.c ../worldmap.c ../worldmap_stdio.c ../savegame.c ../random.c ../bcd4.c ../globalflags.c ../item.c && ./test_combat
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

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

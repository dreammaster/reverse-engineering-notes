/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_combat test_combat.c ../combat.c ../party.c ../monster.c ../monster_stdio.c ../savegame.c ../random.c ../bcd4.c ../item.c && ./test_combat
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

int main(void) {
    testTurnOrderSortedDescending();
    testStableTiesKeepBuildOrder();
    testIncapacitatedPartyMembersExcluded();
    testNoLivingPartyLeavesMonsterUntargeted();
    testSelectActiveMonster();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

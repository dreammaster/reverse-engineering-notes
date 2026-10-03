/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_relics test_relics.c ../relics.c ../globalflags.c ../monster.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_relics
 */
#include <stdio.h>
#include <string.h>

#include "globalflags.h"
#include "monster.h"
#include "party.h"
#include "relics.h"

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void testReadyFlag(void) {
    uint8_t flags[64];
    memset(flags, 0, sizeof(flags));
    check("not ready until flag 0xB1 is set", !relicReady(flags, sizeof(flags)));
    globalFlagSet(flags, sizeof(flags), RelicRechargeFlag);
    check("ready once it is", relicReady(flags, sizeof(flags)));
}

static void testCaches(void) {
    Bcd4 counter, expected;
    bcd4FromU16(counter, 123);
    relicAddCache(counter);
    bcd4FromU16(expected, 5123);
    check("a cache adds 5000", bcd4Compare(counter, expected) == 0);
}

static void testMassHeal(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 0 * 2, 1);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 1 * 2, 2);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 2 * 2, 0);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 3 * 2, 3);
    uint8_t *a = saveGamePartyRecordById(&save, 1);
    uint8_t *dead = saveGamePartyRecordById(&save, 2);
    uint8_t *beyond = saveGamePartyRecordById(&save, 3);
    partySetStatMax(a, PartyStatHitPoints, 50);
    partySetStat(a, PartyStatHitPoints, 3);
    partySetStatMax(a, PartyStatMagicPoints, 20);
    partySetU16(a, PartyFieldStatusFlags, PartyStatusPoisoned | PartyStatusCursed | 0x0005);
    partySetStatMax(dead, PartyStatHitPoints, 40);
    partySetStatMax(dead, PartyStatMagicPoints, 0);
    partySetU16(dead, PartyFieldStatusFlags, PartyStatusDead);
    partySetStatMax(beyond, PartyStatHitPoints, 40);
    partySetStat(beyond, PartyStatHitPoints, 1);

    unsigned n = relicMassHealAndOverheal(&save);

    checkU32("two members reached; the scan stops at the empty slot", n, 2);
    checkU32("HP is twice the maximum", partyGetStat(a, PartyStatHitPoints), 100);
    checkU32("MP too", partyGetStat(a, PartyStatMagicPoints), 40);
    checkU32("every condition above the low six is cleared, the class bits stay", partyGetU16(a, PartyFieldStatusFlags), 0x0005);
    checkU32("death is cured", partyGetU16(dead, PartyFieldStatusFlags), 0);
    checkU32("a dead member gets 2 x max HP too", partyGetStat(dead, PartyStatHitPoints), 80);
    checkU32("and 2 x 0 MP", partyGetStat(dead, PartyStatMagicPoints), 0);
    checkU32("the member past the gap is untouched", partyGetStat(beyond, PartyStatHitPoints), 1);

    partySetStatMax(a, PartyStatHitPoints, 40000);
    relicMassHealAndOverheal(&save);
    checkU32("the doubling is 16-bit: 40000 x 2 wraps", partyGetStat(a, PartyStatHitPoints), (uint16_t)(40000u * 2));
}

static void testKillAndPotion(void) {
    uint8_t monster[MonsterRecordSize];
    memset(monster, 0, sizeof(monster));
    monsterSetU16(monster, MonsterFieldHealth, 300);
    relicInstantKill(monster);
    checkU32("the engaged monster dies", monsterGetU16(monster, MonsterFieldHealth), 0);

    uint8_t flags[64];
    memset(flags, 0, sizeof(flags));
    check("the potion fails elsewhere", !relicUseLocationPotion(104, 111, flags, sizeof(flags)));
    check("...setting nothing", !globalFlagTest(flags, sizeof(flags), LocationPotionFlag));
    check("it works on its one cell", relicUseLocationPotion(104, 110, flags, sizeof(flags)));
    check("...and sets flag 0x48", globalFlagTest(flags, sizeof(flags), LocationPotionFlag));
}

int main(void) {
    testReadyFlag();
    testCaches();
    testMassHeal();
    testKillAndPotion();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

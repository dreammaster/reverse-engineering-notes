/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_ailment test_ailment.c ../ailment.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_ailment
 */
#include <stdio.h>
#include <string.h>

#include "ailment.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static uint8_t *member(SaveGame *save, unsigned slot, uint16_t id, uint16_t status, uint16_t mp, uint16_t survival) {
    saveHeaderSetU16(save, SaveHeaderPartySlots + slot * 2, id);
    if (id == 0) {
        return NULL;
    }
    uint8_t *r = saveGamePartyRecordById(save, id);
    partySetU16(r, PartyFieldStatusFlags, status);
    partySetStat(r, PartyStatMagicPoints, mp);
    partySetStat(r, PartyStatSurvival, survival);
    partySetU16(r, PartyFieldLevel, 10);
    return r;
}

static void testSeverities(void) {
    uint8_t r[PartyRecordSize];
    memset(r, 0, sizeof(r));
    partySetU16(r, PartyFieldStatusFlags, PartyStatusDiseased | PartyStatusPoisoned | PartyStatusSick);
    check("disease 12 + poison 6 + sick 3", ailmentDiseaseSeverity(r) == 21);
    partySetU16(r, PartyFieldStatusFlags, PartyStatusPoisoned | PartyStatusDead);
    check("an incapacitated member suffers nothing", ailmentDiseaseSeverity(r) == 0);
    partySetU16(r, PartyFieldStatusFlags, PartyStatusCursed | PartyStatusHexed | PartyStatusJinxed);
    check("a member with no magic points is spared the curses", ailmentCurseSeverity(r) == 0);
    partySetStat(r, PartyStatMagicPoints, 1);
    check("curse 16 + hex 8 + jinx 4", ailmentCurseSeverity(r) == 28);
    partySetU16(r, PartyFieldStatusFlags, 0);
    uint16_t expect[][2] = {{55, 12}, {56, 9}, {75, 9}, {76, 6}, {80, 6}, {81, 3}, {100, 3}, {101, 0}, {150, 0}, {151, 0}};
    bool ok = true;
    for (unsigned i = 0; i < sizeof(expect) / sizeof(expect[0]); i++) {
        partySetStat(r, PartyStatSurvival, expect[i][0]);
        ok = ok && ailmentSlowSeverity(r) == expect[i][1];
    }
    check("the slow pass tiers Survival (lower skill, bigger hit)", ok);
    partySetU16(r, PartyFieldStatusFlags, PartyStatusDead);
    partySetStat(r, PartyStatSurvival, 10);
    check("...and spares the dead", ailmentSlowSeverity(r) == 0);
}

static void testChapter2Timing(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    member(&save, 0, 1, PartyStatusPoisoned, 5, 60);
    member(&save, 1, 2, 0, 5, 60);
    member(&save, 2, 3, PartyStatusCursed, 5, 60);
    AilmentClock clock = {0, 0};
    AilmentPass passes[4];
    RandomState rng;
    randomStart(&rng, 1, 1);
    check("the UI gate stops everything", ailmentTickIconBar(&clock, &save, GameYendor2, 0, 0, &rng, passes) == 0 && clock.fast == 0);
    unsigned ran = 0;
    for (int i = 1; i <= 39; i++) {
        ran += ailmentTickIconBar(&clock, &save, GameYendor2, 0x0800, 0, &rng, passes);
    }
    check("nothing runs for the first 39 calls", ran == 0 && clock.fast == 39);
    unsigned n = ailmentTickIconBar(&clock, &save, GameYendor2, 0x0800, 0, &rng, passes);
    check("the 40th runs the disease and curse passes", n == 2 && passes[0].kind == AilmentPassDisease && passes[1].kind == AilmentPassCurse &&
                                                          clock.fast == 1);
    check("only the poisoned member is staged in the disease pass (effect 2, severity 6)",
          passes[0].count == 1 && passes[0].stages[0].partyId == 1 && passes[0].stages[0].effectId == 2 && passes[0].stages[0].severity == 6);
    check("only the cursed member in the curse pass (effect 0xE, severity 16)",
          passes[1].count == 1 && passes[1].stages[0].partyId == 3 && passes[1].stages[0].effectId == 0xE && passes[1].stages[0].severity == 16 &&
              passes[1].stages[0].slot == 2);

    clock.fast = 0;
    n = ailmentTickIconBar(&clock, &save, GameYendor2, 0x1000, 2, &rng, passes);
    check("a travel-ailment place adds the slow pass on every call", n == 1 && passes[0].kind == AilmentPassSlow && passes[0].count == 3 &&
                                                                          passes[0].stages[0].severity == 9);
    saveHeaderSetU16(&save, SaveHeaderPartySlots + 2, 0);
    member(&save, 1, 0, 0, 0, 0);
    n = ailmentTickIconBar(&clock, &save, GameYendor2, 0x1000, 2, &rng, passes);
    check("an empty slot ends the scan", passes[0].count == 1);
}

static void testChapter3(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor3);
    member(&save, 0, 1, 0, 0, 0);
    uint8_t *second = member(&save, 1, 2, 0, 0, 0);
    member(&save, 2, 3, PartyStatusParalyzed, 0, 0);
    uint8_t *fourth = member(&save, 3, 4, PartyStatusSick, 0, 0);
    partySetU16(second, AilmentColdProtectionSlot, AilmentColdProtectionItem);
    (void)fourth;
    AilmentClock clock = {0, 0};
    AilmentPass passes[4];
    RandomState rng, peek;
    randomStart(&rng, 3, 4);
    peek = rng;
    unsigned picked = randomInRange(&peek, 3) + 1;
    unsigned ran = 0;
    for (int i = 0; i < 40; i++) {
        ran += ailmentTickIconBar(&clock, &save, GameYendor3, 0x0800, 1, &rng, passes);
    }
    check("40 calls run nothing in Chapter 3 (the counter must exceed 40)", ran == 0);
    unsigned n = ailmentTickIconBar(&clock, &save, GameYendor3, 0x0800, 1, &rng, passes);
    check("the 41st runs disease, curse and cold", n == 3 && passes[0].kind == AilmentPassDisease && passes[1].kind == AilmentPassCurse &&
                                                      passes[2].kind == AilmentPassCold && clock.fast == 1 && clock.slow == 1);
    check("the sick member (party id 4) is staged in the disease pass", passes[0].count == 1 && passes[0].stages[0].partyId == 4 &&
                                                                            passes[0].stages[0].severity == 3);
    const AilmentPass *cold = &passes[2];
    bool shape = true, pickedFound = false;
    for (unsigned i = 0; i < cold->count; i++) {
        const AilmentStage *s = &cold->stages[i];
        shape = shape && s->partyId != 2 && s->partyId != 3; /* fur-wearer and paralysed member are exempt */
        if (s->effectId == AilmentEffectColdPicked) {
            pickedFound = true;
            shape = shape && s->slot == 4 - picked && s->magnitude == 14;
        } else {
            shape = shape && s->effectId == AilmentEffectCold;
        }
    }
    check("the cold pass spares the fur-wearer and the paralysed, and marks the rolled member", shape);
    check("(the rolled member was eligible or not -- consistent either way)", pickedFound == (picked == 4 || picked == 1));
}

int main(void) {
    testSeverities();
    testChapter2Timing();
    testChapter3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

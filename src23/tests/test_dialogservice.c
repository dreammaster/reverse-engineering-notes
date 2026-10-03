/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_dialogservice test_dialogservice.c ../dialogservice.c ../dialog.c ../globalflags.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_dialogservice
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd4.h"
#include "dialogservice.h"
#include "globalflags.h"
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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void putU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)(value & 0xFF);
    record[offset + 1] = (uint8_t)(value >> 8);
}

static void setupParty(SaveGame *save, GameKind game) {
    saveGameInit(save, game);
    for (unsigned slot = 0; slot < 4; slot++) {
        saveHeaderSetU16(save, SaveHeaderPartySlots + slot * 2, slot < 3 ? slot + 1 : 0);
    }
    /* slot 3 left unoccupied; a fourth record exists but sits past the gap */
    saveHeaderSetU16(save, SaveHeaderPartySlots + 3 * 2, 0);
}

static void testAttributeTome(void) {
    SaveGame save;
    setupParty(&save, GameYendor2);
    uint8_t flags[512];
    memset(flags, 0, sizeof(flags));
    uint8_t npc[DialogNpcRecordSize];
    memset(npc, 0, sizeof(npc));
    putU16(npc, DialogNpcOneTimeFlag, 9);
    putU16(npc, DialogNpcParamA, 0x3C); /* PartyStatStrength, current */
    putU16(npc, DialogNpcParamB, 10);

    uint8_t *a = saveGamePartyRecordById(&save, 1);
    uint8_t *dead = saveGamePartyRecordById(&save, 2);
    uint8_t *untrained = saveGamePartyRecordById(&save, 3);
    partySetU16(a, 0x3C, 50);
    partySetU16(a, 0x7C, 60);
    partySetU16(dead, 0x3C, 50);
    partySetU16(dead, 0x7C, 60);
    partySetU16(dead, PartyFieldStatusFlags, PartyStatusDead);
    partySetU16(untrained, 0x3C, 0);
    partySetU16(untrained, 0x7C, 5);

    DialogTomeResult r = dialogApplyAttributeTome(npc, &save, flags, sizeof(flags));
    check("applied the first time", r.applied);
    checkU32("two members were considered and one changed", r.members, 1);
    checkU32("current stat +10", partyGetU16(a, 0x3C), 60);
    checkU32("maximum +10", partyGetU16(a, 0x7C), 70);
    checkU32("a dead member is skipped", partyGetU16(dead, 0x3C), 50);
    checkU32("a zero (untrained) stat is skipped", partyGetU16(untrained, 0x3C), 0);
    check("the one-time flag is set", globalFlagTest(flags, sizeof(flags), 9));

    r = dialogApplyAttributeTome(npc, &save, flags, sizeof(flags));
    check("the second use does nothing", !r.applied && r.members == 0);
    checkU32("...and changes nothing", partyGetU16(a, 0x3C), 60);
}

static void testAttributeCaps(void) {
    SaveGame save;
    setupParty(&save, GameYendor2);
    uint8_t flags[512];
    memset(flags, 0, sizeof(flags));
    uint8_t npc[DialogNpcRecordSize];
    memset(npc, 0, sizeof(npc));
    putU16(npc, DialogNpcOneTimeFlag, 4);
    putU16(npc, DialogNpcParamA, 0x3C);
    putU16(npc, DialogNpcParamB, 10);
    uint8_t *a = saveGamePartyRecordById(&save, 1);
    partySetU16(a, 0x3C, 995);
    partySetU16(a, 0x7C, 990);
    dialogApplyAttributeTome(npc, &save, flags, sizeof(flags));
    checkU32("an ordinary stat is capped at 999", partyGetU16(a, 0x3C), 999);
    checkU32("...and so is its maximum", partyGetU16(a, 0x7C), 999);

    memset(flags, 0, sizeof(flags));
    putU16(npc, DialogNpcParamA, 0x52); /* current HP: capped at 9999 */
    partySetU16(a, 0x52, 9995);
    partySetU16(a, 0x92, 5000);
    dialogApplyAttributeTome(npc, &save, flags, sizeof(flags));
    checkU32("current HP caps at 9999", partyGetU16(a, 0x52), 9999);
    checkU32("max HP just adds", partyGetU16(a, 0x92), 5010);
}

static void testExperienceTome(void) {
    SaveGame save;
    setupParty(&save, GameYendor2);
    uint8_t flags[512];
    memset(flags, 0, sizeof(flags));
    uint8_t npc[DialogNpcRecordSize];
    memset(npc, 0, sizeof(npc));
    putU16(npc, DialogNpcOneTimeFlag, 12);
    bcd4FromU16(npc + DialogNpcParamA, 250);

    uint8_t *a = saveGamePartyRecordById(&save, 1);
    uint8_t *dead = saveGamePartyRecordById(&save, 2);
    partySetU16(dead, PartyFieldStatusFlags, PartyStatusDead);
    bcd4FromU16(a + PartyFieldExperience, 100);

    DialogTomeResult r = dialogApplyExperienceTome(npc, &save, GameYendor2, flags, sizeof(flags));
    check("applied", r.applied);
    Bcd4 expected;
    bcd4FromU16(expected, 350);
    check("experience +250", bcd4Compare(a + PartyFieldExperience, expected) == 0);
    Bcd4 zero;
    bcd4FromU16(zero, 0);
    check("the dead member gets none", bcd4Compare(dead + PartyFieldExperience, zero) == 0);
    check("the flag is set", globalFlagTest(flags, sizeof(flags), 12));
    r = dialogApplyExperienceTome(npc, &save, GameYendor2, flags, sizeof(flags));
    check("one-time", !r.applied);
    check("...so nothing more was added", bcd4Compare(a + PartyFieldExperience, expected) == 0);
}

static void setHp(uint8_t *record, uint16_t hp, uint16_t max) {
    partySetStat(record, PartyStatHitPoints, hp);
    partySetStatMax(record, PartyStatHitPoints, max);
}

static void testClassifyCondition(void) {
    uint8_t record[PartyRecordSize];
    DialogState state;
    memset(record, 0, sizeof(record));
    memset(&state, 0, sizeof(state));
    state.availA = 0x00A5;
    setHp(record, 30, 30);
    dialogClassifyCondition(&state, record);
    checkU32("a healthy member is 0x200, keeping the low bits", state.availA, 0x02A5);

    /* The original keeps the low TEN bits (mask 0x3FF), which includes 0x200 itself: a "nothing wrong" mark from
       a previous member survives reclassification until a topic clears it. Reproduced. */
    setHp(record, 10, 30);
    dialogClassifyCondition(&state, record);
    checkU32("a stale 0x200 survives reclassifying a hurt member", state.availA, 0x82A5);

    state.availA = 0x00A5;
    dialogClassifyCondition(&state, record);
    checkU32("hurt alone is 0x8000", state.availA, 0x80A5);

    partySetU16(record, PartyFieldStatusFlags, PartyStatusPoisoned);
    state.availA = 0x00A5;
    dialogClassifyCondition(&state, record);
    checkU32("hurt and afflicted add the combined 0x1000", state.availA, 0xD0A5);

    setHp(record, 30, 30);
    state.availA = 0x00A5;
    dialogClassifyCondition(&state, record);
    checkU32("afflicted alone is 0x4000", state.availA, 0x40A5);

    partySetU16(record, PartyFieldStatusFlags, PartyStatusDead);
    setHp(record, 0, 30);
    state.availA = 0x00A5;
    dialogClassifyCondition(&state, record);
    checkU32("a dead member at 0 HP is dead + hurt, one tier (no 0x1000)", state.availA, 0xA0A5);
}

static void testHealingCosts(void) {
    uint8_t npc[DialogNpcRecordSize], record[PartyRecordSize];
    DialogState state;
    memset(npc, 0, sizeof(npc));
    memset(record, 0, sizeof(record));
    memset(&state, 0, sizeof(state));
    putU16(npc, DialogNpcPriceMultiplier, 3);
    partySetU16(record, PartyFieldLevel, 5);

    Bcd4 cost, expected;
    dialogHealingCost(npc, DialogHealHp, &state, record, cost);
    bcd4FromU16(expected, 300);
    check("HP: 20 x multiplier 3 x level 5", bcd4Compare(cost, expected) == 0);
    dialogHealingCost(npc, DialogHealRevive, &state, record, cost);
    bcd4FromU16(expected, 1500);
    check("revive: 100 x 3 x 5", bcd4Compare(cost, expected) == 0);

    partySetU16(record, PartyFieldStatusFlags, PartyStatusPoisoned | PartyStatusCursed);
    checkU32("affliction prices sum (poisoned 10 + cursed 40)", dialogAfflictionCost(record), 50);
    dialogHealingCost(npc, DialogHealCure, &state, record, cost);
    bcd4FromU16(expected, 750);
    check("cure: 50 x 3 x 5", bcd4Compare(cost, expected) == 0);

    setHp(record, 10, 30);
    dialogClassifyCondition(&state, record);
    dialogHealingCost(npc, DialogHealFull, &state, record, cost);
    bcd4FromU16(expected, 1050);
    check("full: affliction 50 + HP 20 = 70, x 3 x 5", bcd4Compare(cost, expected) == 0);

    partySetU16(record, PartyFieldStatusFlags, 0x0080 | 0x8000 | 0x0100 | 0x0200 | 0x0400 | 0x0800 | 0x1000 | 0x2000 | 0x4000);
    checkU32("every condition priced: 5+10+20+40+50+60+20+30+40", dialogAfflictionCost(record), 275);
}

static void testApplyHealing(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    setHp(record, 0, 40);
    partySetU16(record, PartyFieldLevel, 1);
    partySetU16(record, PartyFieldStatusFlags, PartyStatusDead | PartyStatusCursed | 0x0005);
    Bcd4 cost, gold, expected;
    bcd4FromU16(cost, 300);
    bcd4FromU16(gold, 200);

    check("not enough gold: nothing happens", !dialogApplyHealing(DialogHealRevive, cost, gold, record, GameYendor2));
    checkU32("...HP unchanged", partyGetStat(record, PartyStatHitPoints), 0);
    bcd4FromU16(expected, 200);
    check("...gold unchanged", bcd4Compare(gold, expected) == 0);

    bcd4FromU16(gold, 500);
    check("with the gold, revive works", dialogApplyHealing(DialogHealRevive, cost, gold, record, GameYendor2));
    checkU32("HP is 2", partyGetStat(record, PartyStatHitPoints), 2);
    checkU32("Dead cleared, Cursed kept", partyGetU16(record, PartyFieldStatusFlags), PartyStatusCursed | 0x0005);
    bcd4FromU16(expected, 200);
    check("gold paid", bcd4Compare(gold, expected) == 0);

    bcd4FromU16(gold, 500);
    dialogApplyHealing(DialogHealCure, cost, gold, record, GameYendor2);
    checkU32("cure keeps only the low 7 bits", partyGetU16(record, PartyFieldStatusFlags), 0x0005);

    partySetU16(record, PartyFieldStatusFlags, PartyStatusDead | PartyStatusSick | 0x0045);
    setHp(record, 1, 40);
    bcd4FromU16(gold, 500);
    dialogApplyHealing(DialogHealFull, cost, gold, record, GameYendor2);
    checkU32("full heal: HP to max", partyGetStat(record, PartyStatHitPoints), 40);
    checkU32("...and only the low 6 status bits remain", partyGetU16(record, PartyFieldStatusFlags), 0x0005);
}

static void testChallenge(void) {
    uint8_t npc[DialogNpcRecordSize], record[PartyRecordSize];
    memset(npc, 0, sizeof(npc));
    memset(record, 0, sizeof(record));
    putU16(npc, DialogNpcParamA, 0x88);
    putU16(npc, DialogNpcParamB, 70);
    putU16(npc, DialogNpcPriceMultiplier, 3);
    putU16(npc, DialogNpcCharacterFlagIndex, 1);
    bcd4FromU16(npc + DialogNpcFee, 1000);

    Bcd4 gold, reward, expected;
    bcd4FromU16(gold, 500);
    check("not enough gold for the fee", dialogAttemptChallenge(npc, record, gold, reward) == DialogChallengeNoGold);
    bcd4FromU16(expected, 500);
    check("...gold untouched", bcd4Compare(gold, expected) == 0);

    bcd4FromU16(gold, 5000);
    partySetU16(record, 0x88, 69);
    check("one point short of the threshold loses", dialogAttemptChallenge(npc, record, gold, reward) == DialogChallengeLost);
    bcd4FromU16(expected, 4000);
    check("...the fee is still paid", bcd4Compare(gold, expected) == 0);
    check("...and no flag is set, so the member can retry", !flagBankTest(record + PartyFieldFlagBank10C, 6, 1));

    partySetU16(record, 0x88, 70);
    check("meeting the threshold wins", dialogAttemptChallenge(npc, record, gold, reward) == DialogChallengeWon);
    bcd4FromU16(expected, 6000);
    check("fee refunded 3 times over: 4000 - 1000 + 3000", bcd4Compare(gold, expected) == 0);
    bcd4FromU16(expected, 3000);
    check("the reward is fee x multiplier", bcd4Compare(reward, expected) == 0);
    check("the member's flag is set", flagBankTest(record + PartyFieldFlagBank10C, 6, 1));

    DialogState state;
    memset(&state, 0, sizeof(state));
    state.availA = 0xE0F0;
    dialogMarkServiceAvailability(&state, npc, record);
    checkU32("a member who already won sees only the FINISHED bit", state.availA, 0x10F0);
    uint8_t other[PartyRecordSize];
    memset(other, 0, sizeof(other));
    dialogMarkServiceAvailability(&state, npc, other);
    checkU32("a fresh member also sees the offer bit", state.availA, 0x90F0);
}

static void testTrainingQuote(void) {
    uint8_t npc[DialogNpcRecordSize], record[PartyRecordSize];
    memset(npc, 0, sizeof(npc));
    memset(record, 0, sizeof(record));
    putU16(npc, DialogNpcParamB, 10);
    putU16(npc, DialogNpcPriceMultiplier, 5);
    Bcd4 cost, expected;

    partySetU16(record, PartyFieldLevel, 4);
    check("below the cap there is a quote", dialogTrainingQuote(npc, record, cost));
    bcd4FromU16(expected, 2000);
    check("100 x 5 x level 4", bcd4Compare(cost, expected) == 0);
    partySetU16(record, PartyFieldLevel, 9);
    check("level 9 -> 10 is still allowed", dialogTrainingQuote(npc, record, cost));
    partySetU16(record, PartyFieldLevel, 10);
    check("level 10 -> 11 would pass the cap: no training", !dialogTrainingQuote(npc, record, cost));
}

static void testRiddles(void) {
    check("Chapter 2 riddle 1 is PENTAGON", strcmp(dialogRiddleAnswer(GameYendor2, 1), "PENTAGON") == 0);
    check("Chapter 2 riddle 6 is GAIN", strcmp(dialogRiddleAnswer(GameYendor2, 6), "GAIN") == 0);
    check("Chapter 2 has no riddle 7", dialogRiddleAnswer(GameYendor2, 7) == NULL);
    check("riddle 0 is not a riddle", dialogRiddleAnswer(GameYendor2, 0) == NULL);
    check("Chapter 3 has 11, the last is 70", strcmp(dialogRiddleAnswer(GameYendor3, 11), "70") == 0);
    check("a correct answer", dialogCheckRiddleAnswer(GameYendor2, 3, "WHITE POTION"));
    check("is case-sensitive", !dialogCheckRiddleAnswer(GameYendor2, 3, "white potion"));
    check("needs the whole string", !dialogCheckRiddleAnswer(GameYendor2, 3, "WHITE"));
    check("and no extra", !dialogCheckRiddleAnswer(GameYendor2, 3, "WHITE POTION "));
    check("a bad riddle id never matches", !dialogCheckRiddleAnswer(GameYendor2, 9, "GAIN"));
}

static void testBuyOre(void) {
    Bcd4 gold, magic, nuore, qty, expected;
    bcd4FromU16(gold, 1005);
    bcd4FromU16(magic, 7);
    bcd4FromU16(nuore, 3);
    bcd4FromU16(qty, 101);
    check("1005 gold affords 100 units, not 101", !dialogBuyOre(2, gold, magic, nuore, qty));
    bcd4FromU16(expected, 1005);
    check("...nothing changes", bcd4Compare(gold, expected) == 0);

    bcd4FromU16(qty, 100);
    check("100 units is fine", dialogBuyOre(2, gold, magic, nuore, qty));
    bcd4FromU16(expected, 5);
    check("...costing 1000 gold", bcd4Compare(gold, expected) == 0);
    bcd4FromU16(expected, 107);
    check("...into MAGIC ORE", bcd4Compare(magic, expected) == 0);
    bcd4FromU16(expected, 3);
    check("...leaving NUORE alone", bcd4Compare(nuore, expected) == 0);

    bcd4FromU16(gold, 50);
    bcd4FromU16(qty, 5);
    check("an argument of 3 buys NUORE", dialogBuyOre(3, gold, magic, nuore, qty));
    bcd4FromU16(expected, 8);
    check("...NUORE 3 + 5", bcd4Compare(nuore, expected) == 0);
    bcd4FromU16(expected, 0);
    check("...for exactly 50 gold", bcd4Compare(gold, expected) == 0);
}

static uint8_t *loadFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)len);
    if (data && fread(data, 1, (size_t)len, f) != (size_t)len) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return data;
}

static void testRealItemServices(void) {
    char path[512];
    const char *dir = getenv("YENDOR2_GAME_DIR");
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : "../../yendor2/game");
    size_t size;
    uint8_t *image = loadFile(path, &size);
    if (!image) {
        printf("SKIP real item-service checks (no WORLD.DAT)\n");
        return;
    }
    static DialogCatalog dialogs;
    static ItemCatalog items;
    check("catalogs parse", dialogCatalogParseWorldDat(&dialogs, GameYendor2, image, size) &&
                                itemCatalogParseWorldDat(&items, GameYendor2, image, size));
    free(image);

    const uint8_t *enhancer = dialogNpc(&dialogs, 57); /* "improve weapons and armor from +5 all the way up to +7" */
    check("the +5 -> +7 smith takes STEEL SHIELD +5 and +6", dialogEnhanceEligible(enhancer, &items, itemCatalogRecord(&items, 198)) &&
                                                               dialogEnhanceEligible(enhancer, &items, itemCatalogRecord(&items, 199)));
    check("...but not +4 (id 197) or the unenhanced shield (id 193)",
          !dialogEnhanceEligible(enhancer, &items, itemCatalogRecord(&items, 197)) &&
              !dialogEnhanceEligible(enhancer, &items, itemCatalogRecord(&items, 193)));
    const uint8_t *apprentice = dialogNpc(&dialogs, 9);
    check("the first smith takes WOODEN SHIELD up to +2 (ids 181-183) but not +3 (184)",
          dialogEnhanceEligible(apprentice, &items, itemCatalogRecord(&items, 181)) &&
              dialogEnhanceEligible(apprentice, &items, itemCatalogRecord(&items, 183)) &&
              !dialogEnhanceEligible(apprentice, &items, itemCatalogRecord(&items, 184)));

    Bcd4 cost, expected;
    check("an enhancement quote exists", dialogEnhanceCost(enhancer, &items, 198, cost));
    memcpy(expected, itemBaseValue(itemCatalogRecord(&items, 199)), sizeof(Bcd4));
    bcd4MulPercent(expected, 80);
    check("...it is the NEXT item's base value at the smith's 80%", bcd4Compare(cost, expected) == 0);
    check("a sell topic accepts by class bits", dialogSellAccepts(0xFFFF, itemCatalogRecord(&items, 198)) &&
                                                  !dialogSellAccepts(0x0000, itemCatalogRecord(&items, 198)));
}

int main(void) {
    testAttributeTome();
    testAttributeCaps();
    testExperienceTome();
    testClassifyCondition();
    testHealingCosts();
    testApplyHealing();
    testChallenge();
    testTrainingQuote();
    testRiddles();
    testBuyOre();
    testRealItemServices();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

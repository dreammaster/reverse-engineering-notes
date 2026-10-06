/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_rest test_rest.c ../rest.c ../gameclock.c ../party.c ../savegame.c ../bcd4.c ../item.c ../effect.c ../random.c && ./test_rest
 */
#include <stdio.h>
#include <string.h>

#include "party.h"
#include "rest.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void setU16At(uint8_t *base, size_t offset, uint16_t value) {
    base[offset] = (uint8_t)value;
    base[offset + 1] = (uint8_t)(value >> 8);
}

static void setUpFoodCatalog(ItemCatalog *catalog) {
    memset(catalog, 0, sizeof(*catalog));
    catalog->game = GameYendor2;
    catalog->itemCount = 0x36;
    catalog->consumableCount = 1;
    uint8_t *food = catalog->items + (0x36 - 1) * ItemRecordSize;
    setU16At(food, ItemFieldFlags, ItemFlagConsumable);
    setU16At(food, ItemFieldTargetOffset, 0);
    setU16At(catalog->consumables, ItemTargetSlotFlags * 2, 0); /* single-use */
}

static uint8_t *setUpMember(SaveGame *save, unsigned slot, bool fed, unsigned hp) {
    uint16_t id = (uint16_t)(slot + 1);
    saveHeaderSetU16(save, SaveHeaderPartySlots + slot * 2, id);
    uint8_t *record = saveGamePartyRecordById(save, id);
    memset(record, 0, PartyRecordSize);
    uint8_t *mainGroup = partyInventoryGroup(record, PartyGroupMain);
    inventoryGroupSetWeight(mainGroup, 100);
    if (fed) {
        itemSlotSet(inventoryGroupSlot(mainGroup, 1), 0x36, 0);
    }
    partySetStat(record, PartyStatHitPoints, (uint16_t)hp);
    partySetStatMax(record, PartyStatHitPoints, 100);
    return record;
}

static unsigned g_calls;
static unsigned g_engageOn;

static bool engage(void *ctx) {
    (void)ctx;
    g_calls++;
    return g_engageOn && g_calls == g_engageOn;
}

static void testRefusals(void) {
    static ItemCatalog catalog;
    setUpFoodCatalog(&catalog);
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    GameClock clock = {420, 5, 3, 7};
    uint8_t slots[24] = {0};
    g_calls = 0;
    g_engageOn = 0;
    RestOutcome out = restParty(&save, &clock, &catalog, slots, true, false, false, engage, NULL);
    check("a held item refuses at once, silently", out.refused && !out.noRest && clock.minutes == 420 && g_calls == 0);
    out = restParty(&save, &clock, &catalog, slots, false, true, false, engage, NULL);
    check("the no-rest flag refuses with the message", out.refused && out.noRest && clock.minutes == 420 && g_calls == 0);
    out = restParty(&save, &clock, &catalog, slots, false, false, true, engage, NULL);
    check("a special cell refuses with the message", out.refused && out.noRest);
}

static void testFullRest(void) {
    static ItemCatalog catalog;
    setUpFoodCatalog(&catalog);
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    uint8_t *a = setUpMember(&save, 0, true, 40);
    uint8_t *b = setUpMember(&save, 1, true, 40);
    uint8_t *c = setUpMember(&save, 2, false, 40);
    GameClock clock = {420, 5, 3, 7};
    uint8_t slots[24] = {0};
    g_calls = 0;
    g_engageOn = 0;
    RestOutcome out = restParty(&save, &clock, &catalog, slots, false, false, false, engage, NULL);
    check("eight slices, 480 minutes, shown as hour 8", !out.refused && !out.interrupted && g_calls == 8 && out.minutes == 480 && out.hour == 8 && clock.minutes == 900);
    check("no day change", !out.dayRolled && clock.day == 5);
    check("two of three active members ate: (100 / 3) * 2 = 66%", out.fed == 2 && out.regenPercent == 66);
    check("everyone heals by 66% of the maximum", partyGetStat(a, PartyStatHitPoints) == 100 && partyGetStat(b, PartyStatHitPoints) == 100 && partyGetStat(c, PartyStatHitPoints) == 100);
    check("the food was used up", partyFindItemInRange(a, 0x36, 0x40, &(unsigned){0}) == 0);
}

static void testInterruptedAndRollover(void) {
    static ItemCatalog catalog;
    setUpFoodCatalog(&catalog);
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    uint8_t *a = setUpMember(&save, 0, true, 40);
    GameClock clock = {1000, 30, 12, 7};
    uint8_t slots[24] = {0};
    g_calls = 0;
    g_engageOn = 3;
    RestOutcome out = restParty(&save, &clock, &catalog, slots, false, false, false, engage, NULL);
    check("a monster engages in the third slice: two hours pass, shown as 3", out.interrupted && out.minutes == 120 && out.hour == 3 && clock.minutes == 1120);
    check("no healing and no food eaten when interrupted", partyGetStat(a, PartyStatHitPoints) == 40 && out.fed == 0 && partyFindItemInRange(a, 0x36, 0x40, &(unsigned){0}) == 0x36);

    clock = (GameClock){1100, 30, 12, 7};
    g_calls = 0;
    g_engageOn = 0;
    out = restParty(&save, &clock, &catalog, slots, false, false, false, engage, NULL);
    check("resting across midnight rolls the day, month and year", out.dayRolled && clock.day == 1 && clock.year == 8);
}

int main(void) {
    testRefusals();
    testFullRest();
    testInterruptedAndRollover();
    printf("%s\n", g_failureCount ? "FAILED" : "ALL PASSED");
    return g_failureCount ? 1 : 0;
}

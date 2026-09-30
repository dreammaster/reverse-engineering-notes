/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_maptrigger test_maptrigger.c ../maptrigger.c ../combat.c ../effect.c ../party.c ../item.c ../bcd4.c ../random.c ../savegame.c ../monsterpool.c ../dungeongrid.c ../movement.c ../monster.c ../monster_stdio.c ../worldmap.c ../worldmap_stdio.c ../globalflags.c && ./test_maptrigger
 */
#include <stdio.h>
#include <string.h>

#include "maptrigger.h"
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

static void checkU32(const char *label, unsigned actual, unsigned expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static void checkI32(const char *label, int actual, int expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %d, want %d\n", label, actual, expected);
    }
}

static uint8_t g_record[PartyRecordSize];

static void setU16At(uint8_t *base, size_t offset, uint16_t value) {
    base[offset] = (uint8_t)value;
    base[offset + 1] = (uint8_t)(value >> 8);
}

static void setWord(uint8_t *table, size_t offset, unsigned word, uint16_t value) {
    setU16At(table, offset + word * 2, value);
}

static void testCount(void) {
    checkU32("Chapter 2 has 9 real trigger entries", mapTriggerCount(GameYendor2), 9);
    checkU32("Chapter 3 has 19 real trigger entries", mapTriggerCount(GameYendor3), 19);
}

static void testFindChapter2(void) {
    MapTriggerRecord r;
    check("Chapter 2: world row 19 matches (a default entry)", mapTriggerFind(GameYendor2, 999, 19, &r));
    checkI32("...effect id (rawA)", r.rawA, 12);
    check("...matches on Y, not X (worldX is irrelevant)", !r.matchIsX);
    checkU32("...flags", r.flags, 0x0000);

    check("Chapter 2: world row 65 matches (a teleport entry)", mapTriggerFind(GameYendor2, 0, 65, &r));
    checkU32("...flags", r.flags, 0x4000);

    check("Chapter 2: world row 67 matches (the corrosion entry)", mapTriggerFind(GameYendor2, 0, 67, &r));
    checkU32("...flags", r.flags, 0x0200);
    checkI32("...rawA is the corrosion slot offset", r.rawA, 322);

    check("Chapter 2: an unmatched row finds nothing", !mapTriggerFind(GameYendor2, 0, 500, &r));
}

static void testFindChapter3(void) {
    MapTriggerRecord r;
    check("Chapter 3: world column 332 matches (an X-axis default entry)", mapTriggerFind(GameYendor3, 332, 0, &r));
    check("...matches on X, not Y", r.matchIsX);
    checkU32("...flags include the item-exclusion bit", r.flags, 0x8001);
    checkI32("...effect id (rawA)", r.rawA, 12);

    check("Chapter 3: world row 157 matches (a plain default entry, no exclusion bit)",
          mapTriggerFind(GameYendor3, 0, 157, &r));
    checkU32("...flags", r.flags, 0x0000);
    checkI32("...effect id", r.rawA, 46);
}

static void testDecideTeleportAndAilmentAreNotDecidedFurther(void) {
    MapTriggerRecord r = {0, false, 0x4000, 100, 200};
    MapTriggerDecision d = mapTriggerDecide(GameYendor2, &r);
    check("teleport flag -> MapTriggerNone", d.outcome == MapTriggerNone);

    r.flags = 0x2000;
    d = mapTriggerDecide(GameYendor2, &r);
    check("ailment-tick flag -> MapTriggerNone", d.outcome == MapTriggerNone);
}

static void testDecideFixedGoldOreIds(void) {
    MapTriggerRecord r = {0, false, 0x1000, 0, 0};
    MapTriggerDecision d = mapTriggerDecide(GameYendor2, &r);
    check("0x1000 -> ApplyEffect", d.outcome == MapTriggerApplyEffect);
    checkU32("...effect id 0xF (gold theft)", d.effectId, 0xF);

    r.flags = 0x0800;
    d = mapTriggerDecide(GameYendor2, &r);
    checkU32("0x800 -> effect id 0x10 (ore1 theft)", d.effectId, 0x10);

    r.flags = 0x0400;
    d = mapTriggerDecide(GameYendor2, &r);
    checkU32("0x400 -> effect id 0x11 (ore2 theft)", d.effectId, 0x11);
}

static void testDecideCorrosionEffectIdSelection(void) {
    MapTriggerRecord r = {0, false, 0x0100, 0x142, 0}; /* bit 0x200 clear -> destroy (effect id 1) */
    MapTriggerDecision d = mapTriggerDecide(GameYendor2, &r);
    check("0x100 alone -> ApplyCorrosion", d.outcome == MapTriggerApplyCorrosion);
    checkU32("...slot offset from rawA", d.slotOffset, 0x142);
    checkU32("...destroy mode (EffectModeItemDestroy)", d.corrosionModeFlags, 0x0200);

    r.flags = 0x0300; /* both bits -- 0x200 set -> replace (effect id 0x2B) */
    d = mapTriggerDecide(GameYendor2, &r);
    checkU32("0x200 set -> replace mode (EffectModeItemReplace)", d.corrosionModeFlags, 0x0400);
}

static void testDecideDefaultBranchAndChapter3Exclusion(void) {
    MapTriggerRecord r = {0, false, 0x0000, 46, 0};
    MapTriggerDecision d = mapTriggerDecide(GameYendor2, &r);
    check("no bits set -> ApplyEffect (fully data-driven)", d.outcome == MapTriggerApplyEffect);
    checkU32("...effect id straight from rawA", d.effectId, 46);
    check("Chapter 2 never requires the item exclusion", !d.requiresItemExclusion);

    r.flags = 0x8001; /* bit 0x8000 is just the axis selector, doesn't gate the dispatch itself */
    d = mapTriggerDecide(GameYendor3, &r);
    check("Chapter 3, bit 0x1 set -> requires the item exclusion", d.requiresItemExclusion);

    r.flags = 0x8000;
    d = mapTriggerDecide(GameYendor3, &r);
    check("Chapter 3, bit 0x1 clear -> no exclusion required", !d.requiresItemExclusion);
}

static void testApplyEffectPlainMagnitude(void) {
    memset(g_record, 0, sizeof(g_record));
    partySetStat(g_record, PartyStatHitPoints, 40);
    partySetStatMax(g_record, PartyStatHitPoints, 100);

    /* Effect id 2 in both games costs HP with a rolled magnitude range -- but rawA/rawB here bypass any
       roll since mapTriggerApplyEffect passes rawA straight through as an already-resolved HP cost. */
    mapTriggerApplyEffect(g_record, NULL, GameYendor2, 2, 5, 0, false);
    checkU32("plain HP-cost effect: HP reduced by rawA", partyGetStat(g_record, PartyStatHitPoints), 35);
}

static void testApplyEffectItemExclusionSkips(void) {
    memset(g_record, 0, sizeof(g_record));
    partySetStat(g_record, PartyStatHitPoints, 40);
    partySetU16(g_record, 0x158, 0x275); /* the excluded item, in the checked slot */

    mapTriggerApplyEffect(g_record, NULL, GameYendor3, 2, 5, 0, true);
    checkU32("excluded member: effect skipped entirely, HP untouched", partyGetStat(g_record, PartyStatHitPoints),
             40);

    partySetU16(g_record, 0x158, 0); /* no longer excluded */
    mapTriggerApplyEffect(g_record, NULL, GameYendor3, 2, 5, 0, true);
    checkU32("...same member, item gone: effect now applies", partyGetStat(g_record, PartyStatHitPoints), 35);
}

static uint32_t bcdHex(const uint8_t *value) {
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static void testApplyEffectGoldTheftBuildsBcd4(void) {
    /*
     * Synthetic -- no real trigger entry in either game exercises the gold/ore-theft branches, so there's
     * no real data to verify against. This checks the Bcd4 construction mechanism itself: rawA's own two
     * bytes become the high digit pair, rawB's the low pair, matching ResolveAttackerActionOutcome's own
     * already-established gold-theft convention (see maptrigger.c's own comment).
     */
    static SaveGame save;
    memset(&save, 0, sizeof(save));
    save.kind = GameYendor2;
    uint8_t *gold = saveHeaderBcd4(&save, SaveHeaderGold);
    gold[0] = 0x12;
    gold[1] = 0x34;
    gold[2] = 0x56;
    gold[3] = 0x78; /* starting gold: 12345678 */

    memset(g_record, 0, sizeof(g_record));
    /* rawA=0x0100 -> bytes {0x01, 0x00}; rawB=0x0000 -> bytes {0x00, 0x00}: a BCD amount of 01000000 */
    mapTriggerApplyEffect(g_record, &save, GameYendor2, 0xF, 0x0100, 0x0000, false);
    checkU32("gold theft builds a Bcd4 from rawA (high pair)/rawB (low pair) and subtracts it",
             bcdHex(saveHeaderBcd4(&save, SaveHeaderGold)), 0x12345678 - 0x01000000);
}

static void testApplyCorrosionEmptySlotIsNoOp(void) {
    ItemCatalog emptyCatalog;
    memset(&emptyCatalog, 0, sizeof(emptyCatalog));

    memset(g_record, 0, sizeof(g_record));
    MapTriggerDecision d = {MapTriggerApplyCorrosion, 0, 0x142, 0x0400, false};
    mapTriggerApplyCorrosion(g_record, &emptyCatalog, GameYendor2, &d);
    checkU32("empty slot: no-op, stays empty", partyGetU16(g_record, 0x142), 0);
}

static void testApplyCorrosionUnknownItemIsNoOp(void) {
    ItemCatalog emptyCatalog;
    memset(&emptyCatalog, 0, sizeof(emptyCatalog));

    memset(g_record, 0, sizeof(g_record));
    partySetU16(g_record, 0x142, 5); /* an item id the (empty) catalog doesn't recognize */
    MapTriggerDecision d = {MapTriggerApplyCorrosion, 0, 0x142, 0x0400, false};
    mapTriggerApplyCorrosion(g_record, &emptyCatalog, GameYendor2, &d);
    checkU32("unknown item id: no-op, slot unchanged", partyGetU16(g_record, 0x142), 5);
}

static void testApplyCorrosionReplacesEquippedItem(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.itemCount = 2;
    catalog.weaponCount = 1;

    /* Item 1: category A (weapon-eligible), corrosion replacement -> item 2. */
    uint8_t *item1 = catalog.items + 0 * ItemRecordSize;
    setU16At(item1, ItemFieldFlags, ItemFlagEquipCode0A);
    setU16At(item1, ItemFieldTargetOffset, 0);
    setWord(catalog.weapons, 0, ItemTargetBreakChanceA, 500);
    setWord(catalog.weapons, 0, ItemTargetBreakItemA, 2);

    memset(g_record, 0, sizeof(g_record));
    /* Slot offset 0x142 is a 4-byte id+extra slot (equipment code 0xC); item 1 equipped there. */
    itemSlotSet(g_record + 0x142, 1, 0);

    MapTriggerDecision d = {MapTriggerApplyCorrosion, 0, 0x142, 0x0400 /* EffectModeItemReplace */, false};
    mapTriggerApplyCorrosion(g_record, &catalog, GameYendor2, &d);

    checkU32("corrosion replaces the equipped item", itemSlotId(g_record + 0x142), 2);
    checkU32("...and parks the corroded item's own id as the slot's extra field", itemSlotExtra(g_record + 0x142),
             1);
}

int main(void) {
    testCount();
    testFindChapter2();
    testFindChapter3();
    testDecideTeleportAndAilmentAreNotDecidedFurther();
    testDecideFixedGoldOreIds();
    testDecideCorrosionEffectIdSelection();
    testDecideDefaultBranchAndChapter3Exclusion();
    testApplyEffectPlainMagnitude();
    testApplyEffectItemExclusionSkips();
    testApplyEffectGoldTheftBuildsBcd4();
    testApplyCorrosionEmptySlotIsNoOp();
    testApplyCorrosionUnknownItemIsNoOp();
    testApplyCorrosionReplacesEquippedItem();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

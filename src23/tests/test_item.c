/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_item test_item.c ../item.c ../item_stdio.c && ./test_item
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "item.h"
#include "item_stdio.h"
#include "party.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

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

/* An item effect adjusts a party-record field: a protection, a current stat or a stat maximum. */
static bool isPartyEffectField(unsigned field) {
    bool protection = field >= PartyFieldProtections && field < PartyFieldProtections + 2 * PartyProtectionCount;
    bool stat = field >= PartyFieldStats && field < PartyFieldStats + 2 * PartyStatCount;
    bool statMax = field >= PartyFieldStatsMax && field < PartyFieldStatsMax + 2 * PartyStatCount;
    return (protection || stat || statMax) && field % 2 == 0;
}

static ItemCatalog g_catalog;
static uint8_t g_region[64 * 1024];

static void testLayouts(void) {
    for (int g = 0; g < 2; g++) {
        const ItemCatalogLayout *layout = itemCatalogLayout((GameKind)g);
        uint32_t sum = layout->itemCount * ItemRecordSize + layout->effectCount * ItemEffectSize +
                       layout->wearableCount * ItemWearableSize + layout->consumableCount * ItemConsumableSize +
                       layout->weaponCount * ItemWeaponSize;
        char label[64];
        snprintf(label, sizeof(label), "%s: tables add up to the region size", g == 0 ? "yendor2" : "yendor3");
        checkU32(label, sum, layout->totalSize);
        snprintf(label, sizeof(label), "%s: counts fit the catalog arrays", g == 0 ? "yendor2" : "yendor3");
        check(label, layout->itemCount <= ItemCountMax && layout->effectCount <= ItemEffectCountMax &&
                         layout->wearableCount <= ItemWearableCountMax &&
                         layout->consumableCount <= ItemConsumableCountMax && layout->weaponCount <= ItemWeaponCountMax);
    }
    checkU32("yendor2 item table starts at 0x71138", itemCatalogLayout(GameYendor2)->itemsOffset, 0x71138);
    checkU32("yendor2 region ends at 0x7E5BA",
             itemCatalogLayout(GameYendor2)->itemsOffset + itemCatalogLayout(GameYendor2)->totalSize, 0x7E5BA);
    checkU32("yendor3 item table starts at 0x83EE8", itemCatalogLayout(GameYendor3)->itemsOffset, 0x83EE8);
    checkU32("yendor3 region ends at 0x8F00A",
             itemCatalogLayout(GameYendor3)->itemsOffset + itemCatalogLayout(GameYendor3)->totalSize, 0x8F00A);
}

static void testParse(void) {
    const ItemCatalogLayout *layout = itemCatalogLayout(GameYendor2);
    for (uint32_t i = 0; i < layout->totalSize; i++) {
        g_region[i] = (uint8_t)(i * 7 + 3);
    }
    check("region shorter than the layout is rejected",
          !itemCatalogParse(&g_catalog, GameYendor2, g_region, layout->totalSize - 1));
    check("exact-size region parses", itemCatalogParse(&g_catalog, GameYendor2, g_region, layout->totalSize));

    check("items come first", memcmp(g_catalog.items, g_region, ItemRecordSize) == 0);
    uint32_t effectsAt = layout->itemCount * ItemRecordSize;
    check("effects follow the items", memcmp(g_catalog.effects, g_region + effectsAt, ItemEffectSize) == 0);
    uint32_t wearablesAt = effectsAt + layout->effectCount * ItemEffectSize;
    check("wearables follow the effects", memcmp(g_catalog.wearables, g_region + wearablesAt, ItemWearableSize) == 0);
    uint32_t consumablesAt = wearablesAt + layout->wearableCount * ItemWearableSize;
    check("consumables follow the wearables",
          memcmp(g_catalog.consumables, g_region + consumablesAt, ItemConsumableSize) == 0);
    uint32_t weaponsAt = consumablesAt + layout->consumableCount * ItemConsumableSize;
    check("weapons come last and end the region",
          memcmp(g_catalog.weapons + (layout->weaponCount - 1) * ItemWeaponSize,
                 g_region + layout->totalSize - ItemWeaponSize, ItemWeaponSize) == 0 &&
              weaponsAt + layout->weaponCount * ItemWeaponSize == layout->totalSize);

    check("id 0 has no record", itemCatalogRecord(&g_catalog, 0) == NULL);
    check("id 1 is the first record", itemCatalogRecord(&g_catalog, 1) == g_catalog.items);
    check("id 759 is the last record", itemCatalogRecord(&g_catalog, 759) == g_catalog.items + 758 * ItemRecordSize);
    check("id 760 has no record", itemCatalogRecord(&g_catalog, 760) == NULL);

    static uint8_t world[0x7E5BA + 16];
    memcpy(world + layout->itemsOffset, g_region, layout->totalSize);
    check("whole-file parse finds the region",
          itemCatalogParseWorldDat(&g_catalog, GameYendor2, world, sizeof(world)) &&
              memcmp(g_catalog.items, g_region, ItemRecordSize) == 0);
    check("a WORLD.DAT that ends inside the region is rejected",
          !itemCatalogParseWorldDat(&g_catalog, GameYendor2, world, layout->itemsOffset + layout->totalSize - 1));
}

static void setLine(uint8_t *record, unsigned offset, const char *text) {
    memset(record + offset, ' ', 12);
    memcpy(record + offset, text, strlen(text));
    record[offset + 12] = 0;
}

static void testNames(void) {
    uint8_t record[ItemRecordSize];
    char name[ItemNameBufferSize];
    char line[ItemNameLineSize];
    memset(record, 0, sizeof(record));

    setLine(record, ItemFieldName1, "BREAD");
    setLine(record, ItemFieldName2, "");
    setLine(record, ItemFieldName3, "");
    itemGetName(record, name);
    check("padding-only trailing lines add nothing", strcmp(name, "BREAD") == 0);

    setLine(record, ItemFieldName1, "WOODEN");
    setLine(record, ItemFieldName2, "SHIELD");
    setLine(record, ItemFieldName3, "+1");
    itemGetName(record, name);
    check("three lines join with single spaces", strcmp(name, "WOODEN SHIELD +1") == 0);

    setLine(record, ItemFieldName1, "ORANGE");
    setLine(record, ItemFieldName2, "");
    setLine(record, ItemFieldName3, "+2");
    itemGetName(record, name);
    check("an empty middle line collapses to one space (trim removes the separator)", strcmp(name, "ORANGE +2") == 0);

    setLine(record, ItemFieldName1, "HALFLING");
    setLine(record, ItemFieldName2, "HELMET OF");
    setLine(record, ItemFieldName3, "INTELLIGENCE");
    itemGetName(record, name);
    check("full 12-character lines fit the buffer", strcmp(name, "HALFLING HELMET OF INTELLIGENCE") == 0);

    memset(record + ItemFieldName1, 'X', 12);
    memset(record + ItemFieldName2, 'Y', 12);
    memset(record + ItemFieldName3, 'Z', 12);
    record[ItemFieldName1 + 12] = record[ItemFieldName2 + 12] = record[ItemFieldName3 + 12] = 0;
    itemGetName(record, name);
    checkU32("longest possible name is 38 characters", (uint32_t)strlen(name), 38);

    itemGetNameLine(record, 1, line);
    check("name line 1", strcmp(line, "YYYYYYYYYYYY") == 0);
    setLine(record, ItemFieldName2, "TWO");
    itemGetNameLine(record, 1, line);
    check("name line is trimmed", strcmp(line, "TWO") == 0);
    itemGetNameLine(record, 3, line);
    check("line 3 does not exist", line[0] == '\0');
}

static void testTargetKind(void) {
    uint8_t record[ItemRecordSize];
    memset(record, 0, sizeof(record));
    struct {
        uint16_t flags;
        ItemTargetKind kind;
        const char *label;
    } cases[] = {
        {0x0000, ItemTargetNone, "no flags: no target"},
        {0x0100, ItemTargetConsumable, "0x100: consumable"},
        {0x0200, ItemTargetWearable, "0x200: wearable"},
        {0x0400, ItemTargetWearable, "0x400 (ring): wearable"},
        {0x0800, ItemTargetWearable, "0x800 (shield): wearable"},
        {0x4000, ItemTargetWeapon, "0x4000: weapon"},
        {0x8000, ItemTargetWeapon, "0x8000: weapon"},
        {0x2000, ItemTargetNone, "0x2000 alone (bags): no target"},
        {0x2004, ItemTargetNone, "a bag's 0x2004: no target"},
        {0x0900, ItemTargetWearable, "wearable outranks consumable"},
        {0x8200, ItemTargetWearable, "wearable outranks weapon"},
        {0x8100, ItemTargetWeapon, "weapon outranks consumable"},
        {0x1000, ItemTargetNone, "alt-icon alone: no target"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        record[ItemFieldFlags] = (uint8_t)cases[i].flags;
        record[ItemFieldFlags + 1] = (uint8_t)(cases[i].flags >> 8);
        check(cases[i].label, itemTargetKind(record) == cases[i].kind);
    }

    itemCatalogParse(&g_catalog, GameYendor2, g_region, itemCatalogLayout(GameYendor2)->totalSize);
    memset(record, 0, sizeof(record));
    record[ItemFieldFlags + 1] = 0x01; /* consumable */
    record[ItemFieldTargetOffset] = (uint8_t)((g_catalog.consumableCount - 1) * ItemConsumableSize);
    record[ItemFieldTargetOffset + 1] = (uint8_t)(((g_catalog.consumableCount - 1) * ItemConsumableSize) >> 8);
    check("last consumable entry is reachable", itemTargetEntry(&g_catalog, record) == g_catalog.consumables +
                                                    (g_catalog.consumableCount - 1) * ItemConsumableSize);
    record[ItemFieldTargetOffset] += ItemConsumableSize;
    check("one entry past the table is NULL", itemTargetEntry(&g_catalog, record) == NULL);

    memset(record, 0, sizeof(record));
    check("no effect offset means no effect", itemEffectEntry(&g_catalog, record) == NULL);
    record[ItemFieldEffectOffset] = ItemEffectSize;
    check("effect offset 16 is the second entry", itemEffectEntry(&g_catalog, record) == g_catalog.effects + 16);
    record[ItemFieldEffectOffset] = 0xFF;
    record[ItemFieldEffectOffset + 1] = 0xFF;
    check("effect offset past the table is NULL", itemEffectEntry(&g_catalog, record) == NULL);

    uint8_t effect[ItemEffectSize] = {0x82, 0, 24, 0, 0x42, 0, 24, 0, 0, 0, 5, 0, 0x2E, 0, 15, 0};
    checkU32("pairs stop at the first zero field", itemEffectPairs(effect), 2);
    checkU32("pair 1 field", itemEffectField(effect, 1), 0x42);
    checkU32("pair 1 amount", itemEffectAmount(effect, 1), 24);
    checkU32("pairs past the end read 0", itemEffectField(effect, 4), 0);
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return itemCatalogReadWorldDatFile(&g_catalog, game, path);
}

static bool nameIs(unsigned id, const char *expected) {
    char name[ItemNameBufferSize];
    const uint8_t *record = itemCatalogRecord(&g_catalog, id);
    if (!record) {
        return false;
    }
    itemGetName(record, name);
    return strcmp(name, expected) == 0;
}

static bool bcdIs(const uint8_t *record, uint32_t hexDigits) {
    const uint8_t *value = itemBaseValue(record);
    return ((uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3]) == hexDigits;
}

/* Every offset a real record carries must land on a whole entry inside its table. */
static void checkInvariants(const char *game) {
    char label[96];
    bool targetsOk = true;
    bool effectsOk = true;
    bool namesOk = true;
    bool pricesOk = true;
    unsigned named = 0;

    for (unsigned id = 1; id <= g_catalog.validItemCount; id++) {
        const uint8_t *record = itemCatalogRecord(&g_catalog, id);
        char name[ItemNameBufferSize];
        itemGetName(record, name);
        if (name[0] == '\0') {
            continue;
        }
        named++;
        for (const char *c = name; *c; c++) {
            if (*c < ' ' || *c > '~') {
                namesOk = false;
            }
        }
        for (int i = 4; i < 8; i++) {
            if ((record[i] >> 4) > 9 || (record[i] & 15) > 9) {
                pricesOk = false;
            }
        }

        ItemTargetKind kind = itemTargetKind(record);
        if (kind != ItemTargetNone) {
            unsigned size = kind == ItemTargetConsumable ? ItemConsumableSize : ItemWearableSize;
            if (itemTargetEntry(&g_catalog, record) == NULL || itemGetU16(record, ItemFieldTargetOffset) % size != 0) {
                targetsOk = false;
            }
        }

        uint16_t effectOffset = itemGetU16(record, ItemFieldEffectOffset);
        if (effectOffset != 0) {
            const uint8_t *effect = itemEffectEntry(&g_catalog, record);
            if (!effect || effectOffset % ItemEffectSize != 0) {
                effectsOk = false;
                continue;
            }
            for (unsigned p = 0; p < itemEffectPairs(effect); p++) {
                unsigned field = itemEffectField(effect, p);
                if (!isPartyEffectField(field) || itemEffectAmount(effect, p) == 0) {
                    effectsOk = false;
                }
            }
        }
    }

    snprintf(label, sizeof(label), "%s: every record with a target has a whole, in-range entry", game);
    check(label, targetsOk);
    snprintf(label, sizeof(label), "%s: every effect offset is a whole entry whose pairs target party-record stats", game);
    check(label, effectsOk);
    snprintf(label, sizeof(label), "%s: every name is printable ASCII", game);
    check(label, namesOk);
    snprintf(label, sizeof(label), "%s: every price is valid packed BCD", game);
    check(label, pricesOk);
    snprintf(label, sizeof(label), "%s: every valid item has a name", game);
    checkU32(label, named, g_catalog.validItemCount);
}

static void testRealYendor2(void) {
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game")) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    checkU32("yendor2 loads 759 records", g_catalog.itemCount, 759);
    checkU32("yendor2 has 744 real items", g_catalog.validItemCount, 744);
    check("id 744 is SCROLL OF RESURRECTION", nameIs(744, "SCROLL OF RESURRECTION"));
    check("id 1 is GOLD COINS", nameIs(1, "GOLD COINS"));
    check("id 2 is MAGIC ORE", nameIs(2, "MAGIC ORE"));
    check("id 6 is BAG", nameIs(6, "BAG"));
    check("id 0x37 is BREAD (item guide)", nameIs(0x37, "BREAD"));
    check("id 0x21E is SLING (page 2, number 0x1E)", nameIs(0x21E, "SLING"));
    check("id 181 is WOODEN SHIELD", nameIs(181, "WOODEN SHIELD"));
    check("id 182 is WOODEN SHIELD +1", nameIs(182, "WOODEN SHIELD +1"));
    check("id 175 is HALFLING HELMET OF INTELLIGENCE", nameIs(175, "HALFLING HELMET OF INTELLIGENCE"));
    check("id 161 keeps the game's SAPHIRE spelling", nameIs(161, "SAPHIRE"));

    const uint8_t *bread = itemCatalogRecord(&g_catalog, 0x37);
    check("BREAD costs 8", bcdIs(bread, 0x00000008));
    checkU32("BREAD weighs 7", itemGetU16(bread, ItemFieldWeight), 7);
    checkU32("BREAD's icon", itemGetU16(bread, ItemFieldIcon), 0xB2);
    checkU32("BREAD's flags", itemGetU16(bread, ItemFieldFlags), 0x100);
    check("BREAD is a consumable", itemTargetKind(bread) == ItemTargetConsumable);
    const uint8_t *breadTarget = itemTargetEntry(&g_catalog, bread);
    check("BREAD's consumable entry is {0, 0, 10, 0}",
          breadTarget && itemTargetWord(breadTarget, 0) == 0 && itemTargetWord(breadTarget, 1) == 0 &&
              itemTargetWord(breadTarget, 2) == 10 && itemTargetWord(breadTarget, 3) == 0);
    check("BREAD has no effect", itemEffectEntry(&g_catalog, bread) == NULL);

    const uint8_t *sling = itemCatalogRecord(&g_catalog, 0x21E);
    check("SLING costs 30", bcdIs(sling, 0x00000030));
    checkU32("SLING weighs 8", itemGetU16(sling, ItemFieldWeight), 8);
    check("SLING is a weapon", itemTargetKind(sling) == ItemTargetWeapon);
    const uint8_t *slingTarget = itemTargetEntry(&g_catalog, sling);
    check("SLING's weapon entry is {1, 0x8A00, 667, 25, 0, 29}",
          slingTarget && itemTargetWord(slingTarget, 0) == 1 && itemTargetWord(slingTarget, 1) == 0x8A00 &&
              itemTargetWord(slingTarget, ItemTargetBreakItemA) == 667 &&
              itemTargetWord(slingTarget, ItemTargetBreakChanceA) == 25 && itemTargetWord(slingTarget, 4) == 0 &&
              itemTargetWord(slingTarget, 5) == 29);

    const uint8_t *shield = itemCatalogRecord(&g_catalog, 181);
    check("WOODEN SHIELD is wearable with absorption 3",
          itemTargetKind(shield) == ItemTargetWearable &&
              itemTargetWord(itemTargetEntry(&g_catalog, shield), ItemTargetAbsorption) == 3);
    checkU32("WOODEN SHIELD fits a backpack and box only", itemGetU16(shield, ItemFieldFitFlags),
             ItemFitBackpack | ItemFitBox);

    const uint8_t *bag = itemCatalogRecord(&g_catalog, 6);
    checkU32("BAG flags are 0x2004", itemGetU16(bag, ItemFieldFlags), 0x2004);
    checkU32("BAG weighs 20", itemGetU16(bag, ItemFieldWeight), 20);
    check("BAG has no target entry", itemTargetEntry(&g_catalog, bag) == NULL);

    /* The effect pairs address party-record fields: 0x82 INTELLIGENCE max, 0x42 current, 0x30 jinxing, 0x2E hexing. */
    const uint8_t *helm = itemCatalogRecord(&g_catalog, 175);
    check("HALFLING HELMET OF INTELLIGENCE costs 22500", bcdIs(helm, 0x00022500));
    const uint8_t *effect = itemEffectEntry(&g_catalog, helm);
    check("...and has four effect pairs", effect && itemEffectPairs(effect) == 4);
    check("...INTELLIGENCE max and current +24, jinxing +30, hexing +15",
          effect && itemEffectField(effect, 0) == 0x82 && itemEffectAmount(effect, 0) == 24 &&
              itemEffectField(effect, 1) == 0x42 && itemEffectAmount(effect, 1) == 24 &&
              itemEffectField(effect, 2) == 0x30 && itemEffectAmount(effect, 2) == 30 &&
              itemEffectField(effect, 3) == 0x2E && itemEffectAmount(effect, 3) == 15);

    const uint8_t *potion = itemEffectEntry(&g_catalog, itemCatalogRecord(&g_catalog, 80));
    check("STRENGTH POTION adds 2 to strength max (0x7C) and current (0x3C)",
          potion && itemEffectPairs(potion) == 2 && itemEffectField(potion, 0) == 0x7C &&
              itemEffectAmount(potion, 0) == 2 && itemEffectField(potion, 1) == 0x3C &&
              itemEffectAmount(potion, 1) == 2);

    checkInvariants("yendor2");
}

static void testRealYendor3(void) {
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game")) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    checkU32("yendor3 loads 631 records", g_catalog.itemCount, 631);
    checkU32("yendor3: all 631 are real items", g_catalog.validItemCount, 631);
    check("id 1 is GOLD COINS", nameIs(1, "GOLD COINS"));
    check("ids are renumbered: id 2 is FOOD", nameIs(2, "FOOD"));
    check("id 3 is NUORE (still an item in chapter 3)", nameIs(3, "NUORE"));
    check("id 6 is CLOTHES +2", nameIs(6, "CLOTHES +2"));
    check("id 100 is SCROLL OF HEALTH", nameIs(100, "SCROLL OF HEALTH"));
    check("id 175 is LEATHER HELMET +3", nameIs(175, "LEATHER HELMET +3"));
    check("id 631 is ORB OF ZAMORA", nameIs(631, "ORB OF ZAMORA"));

    const uint8_t *scroll = itemEffectEntry(&g_catalog, itemCatalogRecord(&g_catalog, 100));
    check("SCROLL OF HEALTH adds 3 to the hit-point maximum field 0x92",
          scroll && itemEffectPairs(scroll) == 1 && itemEffectField(scroll, 0) == 0x92 && itemEffectAmount(scroll, 0) == 3);
    const uint8_t *helm = itemEffectEntry(&g_catalog, itemCatalogRecord(&g_catalog, 175));
    check("LEATHER HELMET +3 protects: 0x30 +30, 0x2E +20, 0x2C +10",
          helm && itemEffectPairs(helm) == 3 && itemEffectField(helm, 0) == 0x30 && itemEffectAmount(helm, 0) == 30 &&
              itemEffectField(helm, 1) == 0x2E && itemEffectAmount(helm, 1) == 20 && itemEffectField(helm, 2) == 0x2C &&
              itemEffectAmount(helm, 2) == 10);
    check("BROKEN SLING (id 7) is a weapon-slot item", itemTargetKind(itemCatalogRecord(&g_catalog, 7)) == ItemTargetWeapon);

    checkInvariants("yendor3");
}

int main(void) {
    testLayouts();
    testParse();
    testNames();
    testTargetKind();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_creation test_creation.c ../creation.c ../chargen.c ../charcreate.c ../textfield.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewrender.c ../font.c \
 *       ../savegame.c ../statsheet.c ../paperdoll.c ../roster.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_creation
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game for the item catalog (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "creation.h"

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

static uint8_t *readFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static void testGame(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    static ItemCatalog catalog;
    if (!itemCatalogParseWorldDat(&catalog, game, world, size)) {
        check("the item catalog parses", false);
        return;
    }
    RandomState rng;
    randomStart(&rng, 30, 50);
    uint16_t offered[CreationItemChoices];
    creationItemIds(game, offered);
    bool allReal = true;
    for (unsigned i = 0; i < CreationItemChoices; i++) {
        allReal = allReal && itemCatalogRecord(&catalog, offered[i]) != NULL && offered[i] <= catalog.validItemCount;
    }
    char text[160];
    snprintf(text, sizeof(text), "%s: the eight creation items are real catalog items", label);
    check(text, allReal);
    bool classesOk = true, magicOk = true, namesOk = true, itemsOk = true, statsOk = true;
    for (unsigned base = 1; base <= 9; base++) {
        uint8_t hero[SavePartyRecordSize];
        CreationChoice choice = {1 + base % 2, 1 + base % 9, base, {0, 3, 3, 8}, "HERO"};
        bool made = creationMakeHero(hero, game, &catalog, &rng, &choice);
        classesOk = classesOk && made && partyGetU16(hero, PartyFieldClass) == base && partyGetU16(hero, PartyFieldLevel) == 1;
        namesOk = namesOk && memcmp(hero + PartyFieldName, "HERO", 5) == 0;
        statsOk = statsOk && partyGetStat(hero, PartyStatStrength) >= 45 && partyGetStat(hero, PartyStatStrength) <= 60 && partyGetStat(hero, PartyStatHitPoints) > 0;
        unsigned held = 0;
        for (unsigned slot = 1; slot <= InventorySlotCount; slot++) {
            held += itemSlotId(inventoryGroupSlot(partyInventoryGroup(hero, PartyGroupMain), slot)) != 0;
        }
        itemsOk = itemsOk && held == 2 && itemSlotId(inventoryGroupSlot(partyInventoryGroup(hero, PartyGroupMain), 1)) == offered[0] &&
                  itemSlotId(inventoryGroupSlot(partyInventoryGroup(hero, PartyGroupMain), 2)) == offered[3];
        if (base >= 4) {
            magicOk = magicOk && partyGetStat(hero, PartyStatMagicPoints) > 0;
        }
    }
    snprintf(text, sizeof(text), "%s: all nine classes can be made (class, level 1, name, rolled stats)", label);
    check(text, classesOk && namesOk && statsOk);
    check("... the repeated and out-of-range item picks count once or not at all", itemsOk);
    check("... the magic classes have magic points", magicOk);
    uint8_t bad[SavePartyRecordSize];
    CreationChoice noName = {1, 1, 1, {8, 8, 8, 8}, "   "}, noClass = {1, 1, 0, {8, 8, 8, 8}, "X"}, noPortrait = {1, 10, 1, {8, 8, 8, 8}, "X"};
    check("an empty name, class 0 and portrait 10 are refused", !creationMakeHero(bad, game, &catalog, &rng, &noName) && !creationMakeHero(bad, game, &catalog, &rng, &noClass) &&
                                                                  !creationMakeHero(bad, game, &catalog, &rng, &noPortrait));
    uint8_t hero[SavePartyRecordSize];
    CreationChoice mage = {2, 4, 7, {8, 8, 8, 8}, "MERLIN-LONGNAME-TOO-LONG"};
    creationMakeHero(hero, game, &catalog, &rng, &mage);
    uint16_t before = partyGetStat(hero, PartyStatStrength), face = partyGetU16(hero, 0x12);
    bool changed = false;
    for (unsigned i = 0; i < 20 && !changed; i++) {
        creationReroll(hero, game, &catalog, &rng);
        changed = partyGetStat(hero, PartyStatStrength) != before;
    }
    check("a long name is cut to 12 characters, R rerolls the attributes and keeps the class and portrait", strlen((const char *)hero + PartyFieldName) == 12 && changed &&
                                                                                                                  partyGetU16(hero, PartyFieldClass) == 7 && partyGetU16(hero, 0x12) == face);
    free(world);
}

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_chest test_chest.c ../chest.c ../savegame.c ../item.c ../bcd4.c ../lockcatalog.c && ./test_chest
 *
 * The real-data check reads WORLD.DAT of yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd4.h"
#include "chest.h"

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

static void setU16At(uint8_t *base, size_t offset, uint16_t value) {
    base[offset] = (uint8_t)value;
    base[offset + 1] = (uint8_t)(value >> 8);
}

static unsigned bcd(const uint8_t *value) {
    return (unsigned)((value[0] >> 4) * 10000000 + (value[0] & 15) * 1000000 + (value[1] >> 4) * 100000 + (value[1] & 15) * 10000 + (value[2] >> 4) * 1000 +
                      (value[2] & 15) * 100 + (value[3] >> 4) * 10 + (value[3] & 15));
}

static void testSynthetic(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.itemCount = 6;
    catalog.consumableCount = 2;
    setU16At(catalog.items + 0 * ItemRecordSize, ItemFieldFlags, 0x80); /* 1: gold */
    setU16At(catalog.items + 1 * ItemRecordSize, ItemFieldFlags, 0x40); /* 2: ore */
    setU16At(catalog.items + 2 * ItemRecordSize, ItemFieldFlags, 0x20); /* 3: nuore */
    uint8_t *potion = catalog.items + 3 * ItemRecordSize;               /* 4: a charged consumable, default 7 charges */
    setU16At(potion, ItemFieldFlags, ItemFlagConsumable);
    setU16At(potion, ItemFieldTargetOffset, 0);
    setU16At(catalog.consumables, ItemTargetSlotFlags * 2, 1);
    setU16At(catalog.consumables, 2 * 2, 7);
    setU16At(catalog.items + 4 * ItemRecordSize, ItemFieldFlags, 0); /* 5: a plain item */

    LockRecord lock;
    memset(&lock, 0, sizeof(lock));
    lock.items[0] = 1;
    lock.items[1] = 2;
    lock.items[2] = 3;
    lock.items[3] = 4;
    lock.items[4] = 5;
    lock.items[6] = 9; /* an id beyond the catalog */
    lock.gold = 250;
    lock.magicOre = 12;
    lock.nuore = 3;

    ChestSlot slots[LockContentSlots];
    chestSlots(&lock, 0, &catalog, slots);
    check("slot 0 is the gold pile with the gold of the record", slots[0].kind == ChestSlotGold && slots[0].value == 250);
    check("slot 1 is the ore pile, slot 2 the nuore pile", slots[1].kind == ChestSlotOre && slots[1].value == 12 && slots[2].kind == ChestSlotNuore && slots[2].value == 3);
    check("a charged consumable shows its default charge", slots[3].kind == ChestSlotItem && slots[3].itemId == 4 && slots[3].value == 7);
    check("a plain item has no value", slots[4].kind == ChestSlotItem && slots[4].value == 0);
    check("an empty slot is empty", slots[5].kind == ChestSlotEmpty && slots[7].kind == ChestSlotEmpty);
    check("an unknown item id is still an item", slots[6].kind == ChestSlotItem && slots[6].itemId == 9);

    chestSlots(&lock, 0x80 | 0x08, &catalog, slots);
    check("the taken byte hides slots MSB first: bit 0x80 is slot 0, 0x08 is slot 4", slots[0].kind == ChestSlotEmpty && slots[4].kind == ChestSlotEmpty && slots[1].kind == ChestSlotOre);

    SaveGame save;
    saveGameInit(&save, GameYendor2);
    ChestSlot taken;
    check("taking the gold pile", chestTake(&save, 5, &lock, &catalog, 0, &taken) && taken.kind == ChestSlotGold && bcd(saveHeaderBcd4(&save, SaveHeaderGold)) == 250);
    check("... persists the bit in lock 5's byte only", chestTakenMask(&save, 5) == 0x80 && chestTakenMask(&save, 4) == 0 && chestTakenMask(&save, 6) == 0);
    check("... and the slot cannot be taken twice", !chestTake(&save, 5, &lock, &catalog, 0, &taken) && bcd(saveHeaderBcd4(&save, SaveHeaderGold)) == 250);
    check("taking the ore pile", chestTake(&save, 5, &lock, &catalog, 1, &taken) && bcd(saveHeaderBcd4(&save, SaveHeaderOreCounter1)) == 12 && chestTakenMask(&save, 5) == 0xC0);
    check("taking an ordinary item leaves the counters alone and hands over its extra",
          chestTake(&save, 5, &lock, &catalog, 3, &taken) && taken.itemId == 4 && taken.value == 7 && bcd(saveHeaderBcd4(&save, SaveHeaderOreCounter2)) == 0);
    check("an empty slot, a bad slot and lock id 0 are refused",
          !chestTake(&save, 5, &lock, &catalog, 5, &taken) && !chestTake(&save, 5, &lock, &catalog, 8, &taken) && !chestTake(&save, 0, &lock, &catalog, 2, &taken));
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

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *data = readFile(path, &size);
    if (!data) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    static ItemCatalog items;
    static LockCatalog locks;
    check(label, itemCatalogParseWorldDat(&items, game, data, size) && lockCatalogParseWorldDat(&locks, game, data, size));
    unsigned withPiles = 0, badPiles = 0, chests = 0;
    for (unsigned id = 1; id <= locks.recordCount; id++) {
        LockRecord lock;
        ChestSlot slots[LockContentSlots];
        if (!lockCatalogRecord(&locks, id, &lock)) {
            continue;
        }
        chestSlots(&lock, 0, &items, slots);
        bool any = false;
        for (unsigned s = 0; s < LockContentSlots; s++) {
            any = any || slots[s].kind != ChestSlotEmpty;
            if (slots[s].kind == ChestSlotGold || slots[s].kind == ChestSlotOre || slots[s].kind == ChestSlotNuore) {
                withPiles++;
                badPiles += slots[s].itemId > 3;
            }
        }
        chests += any;
    }
    printf("     %s: %u lock records with contents, %u pile slots\n", label, chests, withPiles);
    check("the pile items are exactly ids 1-3 in the real catalog", chests > 0 && withPiles > 0 && badPiles == 0);
    free(data);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2 catalogs parse");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3 catalogs parse");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

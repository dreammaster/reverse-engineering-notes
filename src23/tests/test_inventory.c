/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_inventory test_inventory.c ../inventory.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_inventory
 *
 * Real-data checks read WORLD.DAT from yendor2/game (skipped if absent; YENDOR2_GAME_DIR overrides).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inventory.h"
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

static void testEligibility(void) {
    check("inventory slots take anything", inventoryEligibleForSlot(1, 0, 0, false, false, false) && inventoryEligibleForSlot(8, 0, 0, false, false, false));
    check("code 0xA: weapons (0x8000)", inventoryEligibleForSlot(0xA, 0x8000, 0, false, false, false) && !inventoryEligibleForSlot(0xA, 0x4000, 0, false, false, false));
    check("code 0xB: an occupied slot always swaps, an empty one needs 0x2000", inventoryEligibleForSlot(0xB, 0, 0, true, false, false) &&
                                                                                !inventoryEligibleForSlot(0xB, 0, 0, false, false, false) &&
                                                                                inventoryEligibleForSlot(0xB, 0x2000, 0, false, false, false));
    check("code 0xC: 0x4000, a one-handed weapon is fine", inventoryEligibleForSlot(0xC, 0x4000, 0, false, true, false) && !inventoryEligibleForSlot(0xC, 0, 0, false, false, false));
    check("...a two-handed one (word 1 bit 0) needs the 0xD slot empty", !inventoryEligibleForSlot(0xC, 0x4000, 1, false, true, false) &&
                                                                          inventoryEligibleForSlot(0xC, 0x4000, 1, false, false, false));
    check("code 0xD: 0x800 and no two-handed weapon", inventoryEligibleForSlot(0xD, 0x800, 0, false, false, false) &&
                                                      !inventoryEligibleForSlot(0xD, 0x800, 0, false, false, true) && !inventoryEligibleForSlot(0xD, 0, 0, false, false, false));
    check("codes 0xE/0xF: rings (0x400)", inventoryEligibleForSlot(0xE, 0x400, 0, false, false, false) && inventoryEligibleForSlot(0xF, 0x400, 0, false, false, false) &&
                                              !inventoryEligibleForSlot(0xF, 0x200, 0, false, false, false));
    check("code 0x10-0x14 need 0x200 and the matching word-1 bit",
          inventoryEligibleForSlot(0x10, 0x200, 0x8000, false, false, false) && !inventoryEligibleForSlot(0x10, 0, 0x8000, false, false, false) &&
              inventoryEligibleForSlot(0x11, 0x200, 0x4000, false, false, false) && inventoryEligibleForSlot(0x12, 0x200, 0x2000, false, false, false) &&
              inventoryEligibleForSlot(0x13, 0x200, 0x1000, false, false, false) && inventoryEligibleForSlot(0x14, 0x200, 0x800, false, false, false) &&
              !inventoryEligibleForSlot(0x11, 0x200, 0x8000, false, false, false));
    check("code 9 falls into the same generic test", inventoryEligibleForSlot(9, 0x200, 0x800, false, false, false) && !inventoryEligibleForSlot(9, 0, 0x800, false, false, false));
}

static void testContainers(void) {
    check("a BAG takes bag-fitting items", inventoryContainerAccepts(0x2004, 0x2000) && !inventoryContainerAccepts(0x2004, 0x4000));
    check("a BOX takes box-fitting, a BACKPACK backpack-fitting", inventoryContainerAccepts(0x2008, 0x4000) && !inventoryContainerAccepts(0x2008, 0x8000) &&
                                                                  inventoryContainerAccepts(0x2010, 0x8000));
    check("the MAGIC CONTAINER takes anything with any fit flag, nothing otherwise", inventoryContainerAccepts(0x2002, 0x0001) && !inventoryContainerAccepts(0x2002, 0));
    check("a non-container takes nothing", !inventoryContainerAccepts(0x0100, 0xFFFF));
}

static void testDrop(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.itemCount = 10;
    for (unsigned id = 1; id <= 10; id++) {
        memset(catalog.items + (id - 1) * ItemRecordSize, 0, ItemRecordSize);
    }
    /* item 1: plain; item 2: no-drop key; items 3-5: containers; */
    catalog.items[(2 - 1) * ItemRecordSize + ItemFieldFlags] = 0x01;
    for (unsigned id = 3; id <= 5; id++) {
        catalog.items[(id - 1) * ItemRecordSize + ItemFieldFlags + 1] = 0x20;
    }
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    check("a plain item can be dropped", !inventoryDropBlocked(GameYendor2, &catalog, &save, 1, 0));
    check("a key cannot", inventoryDropBlocked(GameYendor2, &catalog, &save, 2, 0));
    check("nothing held is not blocked", !inventoryDropBlocked(GameYendor2, &catalog, &save, 0, 0));
    check("an empty container can be dropped", !inventoryDropBlocked(GameYendor2, &catalog, &save, 3, 20));

    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 20), 3), 1, 0);
    check("a container holding a plain item can", !inventoryDropBlocked(GameYendor2, &catalog, &save, 3, 20));
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 20), 4), 2, 0);
    check("...holding a key cannot", inventoryDropBlocked(GameYendor2, &catalog, &save, 3, 20));

    SaveGame deep;
    saveGameInit(&deep, GameYendor2);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&deep, SaveSectionItemInstances, 20), 1), 4, 21); /* held -> 20 holds container -> 21 */
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&deep, SaveSectionItemInstances, 21), 1), 5, 22);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&deep, SaveSectionItemInstances, 22), 1), 2, 0);
    check("a key two containers down (third level) still blocks", inventoryDropBlocked(GameYendor2, &catalog, &deep, 3, 20));
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&deep, SaveSectionItemInstances, 22), 1), 5, 23);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&deep, SaveSectionItemInstances, 23), 1), 2, 0);
    check("...but one in a fourth-level container is never looked at", !inventoryDropBlocked(GameYendor2, &catalog, &deep, 3, 20));
}

static void testDiscard(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.itemCount = 10;
    for (unsigned id = 3; id <= 5; id++) {
        catalog.items[(id - 1) * ItemRecordSize + ItemFieldFlags + 1] = 0x20; /* containers 3-5; 1-2 plain */
    }
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    /* held container 3 -> record 20 holds a plain item 1 and container 4 -> record 21 holds container 5 -> record 22 holds container 5 -> 23 */
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 20), 1), 1, 0);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 20), 2), 4, 21);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 21), 1), 5, 22);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 22), 1), 5, 23);
    itemSlotSet(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 23), 1), 2, 0);
    inventoryDiscardDropped(&catalog, &save, 1, 20);
    check("dropping a plain item leaves the save alone", itemSlotId(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 20), 1)) == 1);
    inventoryDiscardDropped(&catalog, &save, 3, 20);
    bool zero20 = true, zero21 = true, zero22 = true;
    for (unsigned i = 0; i < InventoryGroupSize; i++) {
        zero20 = zero20 && saveGameRecord(&save, SaveSectionItemInstances, 20)[i] == 0;
        zero21 = zero21 && saveGameRecord(&save, SaveSectionItemInstances, 21)[i] == 0;
        zero22 = zero22 && saveGameRecord(&save, SaveSectionItemInstances, 22)[i] == 0;
    }
    check("dropping a container zeroes its record and the two levels of containers below it", zero20 && zero21 && zero22);
    check("...but not a fourth level", itemSlotId(inventoryGroupSlot(saveGameRecord(&save, SaveSectionItemInstances, 23), 1)) == 2);
}

static void testLocation(void) {
    check("a weapon needs 0x100, armour 0x40", inventoryLocationAccepts(0x8000, 0x100) && !inventoryLocationAccepts(0x8000, 0x40) &&
                                                  inventoryLocationAccepts(0x800, 0x40) && !inventoryLocationAccepts(0x800, 0x100) &&
                                                  inventoryLocationAccepts(0x4000, 0x100));
    check("anything else never", !inventoryLocationAccepts(0x200, 0xFFFF));
}

static uint8_t *loadFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc((size_t)len);
    if (data && fread(data, 1, (size_t)len, f) != (size_t)len) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return data;
}

static void testReal(void) {
    char path[512];
    const char *dir = getenv("YENDOR2_GAME_DIR");
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : "../../yendor2/game");
    size_t size;
    uint8_t *image = loadFile(path, &size);
    if (!image) {
        printf("SKIP real inventory checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }
    static ItemCatalog items;
    check("catalog parses", itemCatalogParseWorldDat(&items, GameYendor2, image, size));
    free(image);
    SaveGame save;
    saveGameInit(&save, GameYendor2);
    unsigned noDrop = 0;
    for (unsigned id = 1; id <= itemCatalogLayout(GameYendor2)->validItemCount; id++) {
        noDrop += inventoryDropBlocked(GameYendor2, &items, &save, (uint16_t)id, 0) && !(itemGetU16(itemCatalogRecord(&items, id), ItemFieldFlags) & 0x2000);
    }
    check("47 items cannot be dropped, the BRASS CHEST KEY among them", noDrop >= 46 && inventoryDropBlocked(GameYendor2, &items, &save, 0x21, 0));
}

int main(void) {
    testEligibility();
    testContainers();
    testDrop();
    testDiscard();
    testLocation();
    testReal();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

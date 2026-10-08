/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_creationwizard test_creationwizard.c ../creationwizard.c ../creation.c ../chargen.c ../charcreate.c ../textfield.c ../party.c ../item.c ../bcd4.c \
 *       ../effect.c ../random.c ../viewrender.c ../font.c ../savegame.c ../statsheet.c ../paperdoll.c ../roster.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_creationwizard
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game for the item catalog (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "creationwizard.h"

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

static void keys(CreationWizard *w, const char *text) {
    for (; *text; text++) {
        creationWizardKey(w, (unsigned char)*text == '^' ? 13 : (unsigned char)*text);
    }
}

static unsigned heldItems(const uint8_t *record) {
    unsigned held = 0;
    uint8_t *group = partyInventoryGroup((uint8_t *)record, PartyGroupMain);
    for (unsigned slot = 1; slot <= InventorySlotCount; slot++) {
        held += itemSlotId(inventoryGroupSlot(group, slot)) != 0;
    }
    return held;
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
    static CreationWizard w;
    char text[160];

    creationWizardStart(&w, game, &catalog, &rng);
    check("the wizard starts on the class step", w.step == CreationStepClass);
    keys(&w, "z");
    check("a key that is no class hotkey is ignored", w.step == CreationStepClass);
    keys(&w, "g");
    snprintf(text, sizeof(text), "%s: G picks the mage (class 7), rolls it and goes on to the portrait", label);
    check(text, w.step == CreationStepPortrait && partyGetU16(w.record, PartyFieldClass) == 7 && partyGetU16(w.record, PartyFieldLevel) == 1 && partyGetStat(w.record, PartyStatStrength) >= 45);
    keys(&w, "f");
    check("F switches to the female portraits", w.gender == 2 && partyGetU16(w.record, 0x10) == 2 && w.step == CreationStepPortrait);
    keys(&w, "3");
    check("a digit picks the portrait: the face and body pictures", w.step == CreationStepRoll && partyGetU16(w.record, 0x14) == charCreateBodyPicture(2, 3) && partyGetU16(w.record, 0x12) == charCreateFacePicture(2, 3));
    unsigned before = partyGetStat(w.record, PartyStatStrength) * 1000 + partyGetStat(w.record, PartyStatDexterity);
    bool changed = false;
    for (unsigned i = 0; i < 8 && !changed; i++) {
        keys(&w, "r");
        changed = partyGetStat(w.record, PartyStatStrength) * 1000 + partyGetStat(w.record, PartyStatDexterity) != before;
    }
    check("R rolls the attributes again", changed && w.step == CreationStepRoll);
    keys(&w, "i");
    check("I accepts the roll and goes to the items", w.step == CreationStepItems && heldItems(w.record) == 0);
    keys(&w, "1");
    keys(&w, "1");
    check("an item is taken once", w.taken == 1 && heldItems(w.record) == 1 && (w.hiddenMask & 0x80));
    keys(&w, "n");
    check("N goes on to the name before four are taken", w.step == CreationStepName);
    keys(&w, "^");
    check("an empty name is not kept", w.step == CreationStepName);
    keys(&w, "bob^");
    check("the name is kept in capitals and leads to the summary", w.step == CreationStepSummary && memcmp(w.record + PartyFieldName, "BOB", 4) == 0);
    keys(&w, "p");
    check("P goes back to the portrait step", w.step == CreationStepPortrait);
    keys(&w, "5");
    check("and returns to the summary", w.step == CreationStepSummary && partyGetU16(w.record, 0x12) == charCreateFacePicture(2, 5));
    keys(&w, "i");
    check("I from the summary starts the item pick over", w.step == CreationStepItems && heldItems(w.record) == 0 && w.taken == 0);
    keys(&w, "1234");
    check("the fourth item ends the pick and returns to the summary", w.step == CreationStepSummary && heldItems(w.record) == 4);
    keys(&w, "nx");
    creationWizardKey(&w, 27);
    check("Escape leaves the name step for the summary keeping the old name", w.step == CreationStepSummary && memcmp(w.record + PartyFieldName, "BOB", 4) == 0);
    bool running = creationWizardKey(&w, 'k');
    snprintf(text, sizeof(text), "%s: K keeps the hero: done, with the class's starting abilities", label);
    check(text, !running && w.step == CreationStepDone && partyGetU16(w.record, PartyFieldClass) == 7);
    creationWizardStart(&w, game, &catalog, &rng);
    running = creationWizardKey(&w, 'q');
    check("Q quits creation", !running && w.step == CreationStepCancelled);
    free(world);
}

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

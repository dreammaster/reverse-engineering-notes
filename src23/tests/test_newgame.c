/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_newgame test_newgame.c ../newgame.c ../savegame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c && ./test_newgame
 *
 * Reads WORLD.DAT from yendor2/game and yendor3/game (gitignored; skipped if absent). Set YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "newgame.h"
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

static void testShortImage(void) {
    SaveGame save;
    uint8_t tiny[100] = {0};
    check("a too-short WORLD.DAT is refused", !saveGameNewGame(&save, GameYendor2, tiny, sizeof(tiny)));
}

static void testSynthetic(void) {
    /* A fake image with a recognisable template at the Chapter 2 offset. */
    size_t size = NewGameTemplateOffsetYendor2 + NewGameTemplateSize;
    uint8_t *image = (uint8_t *)calloc(size, 1);
    uint8_t *tpl = image + NewGameTemplateOffsetYendor2;
    memcpy(tpl, "TEST PARTY", 11);
    tpl[0x1EC] = 6;                 /* a non-empty active slot */
    tpl[500 * 6 + 0x15C + 1] = 0x08; /* record 6's UI flag 0x0800 */
    tpl[500 * 6 + 0x15C] = 0x05;
    static SaveGame save;
    check("the template loads", saveGameNewGame(&save, GameYendor2, image, size));
    check("the name comes from the template", memcmp(saveGameSection(&save, SaveSectionHeaderAndParty), "TEST PARTY", 11) == 0);
    check("the active party is emptied", saveHeaderGetU16(&save, SaveHeaderPartySlots) == 0);
    uint8_t *rec6 = saveGameRecord(&save, SaveSectionHeaderAndParty, 6);
    check("bit 0x0800 of each record's UI flags is cleared, the rest kept", partyGetU16(rec6, PartyFieldUiFlags) == 0x0005);
    check("the opening values are asserted", saveHeaderGetU16(&save, SaveHeaderWorldX) == 166 && saveHeaderGetU16(&save, SaveHeaderWorldY) == 36 &&
                                                 saveHeaderGetU16(&save, SaveHeaderFacing) == SaveFacingWest &&
                                                 saveHeaderGetU16(&save, SaveHeaderClockMinutes) == 420);
    free(image);
}

static void testReal(int chapter) {
    char path[512];
    const char *envName = chapter == 2 ? "YENDOR2_GAME_DIR" : "YENDOR3_GAME_DIR";
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : (chapter == 2 ? "../../yendor2/game" : "../../yendor3/game"));
    size_t size;
    uint8_t *image = loadFile(path, &size);
    if (!image) {
        printf("SKIP chapter %d real new-game checks (no WORLD.DAT)\n", chapter);
        g_skipCount++;
        return;
    }
    GameKind game = chapter == 2 ? GameYendor2 : GameYendor3;
    static SaveGame save;
    check("the real template loads", saveGameNewGame(&save, game, image, size));
    for (unsigned slot = 0; slot < 4; slot++) {
        if (saveHeaderGetU16(&save, SaveHeaderPartySlots + slot * 2) != 0) {
            check("the active party is empty", false);
        }
    }
    const char *name = (const char *)saveGameSection(&save, SaveSectionHeaderAndParty);
    check("the header is named by the template", chapter == 2 ? strcmp(name, "SMITHWARE PARTY") == 0 : strcmp(name, "PRE-CREATED PARTY") == 0);
    static const char *const heroes[4] = {"SQUIRE", "DIANA", "YENDOR", "JOSEPHINE"};
    bool heroesOk = true;
    for (unsigned i = 0; i < 4; i++) {
        const uint8_t *record = saveGameRecord(&save, SaveSectionHeaderAndParty, 6 + i);
        heroesOk = heroesOk && strcmp((const char *)record, heroes[i]) == 0 && partyGetU16(record, PartyFieldClass) != 0;
    }
    check("roster slots 6-9 hold the four ready-made heroes", heroesOk);
    bool emptyOk = true;
    for (unsigned i = 1; i <= 5; i++) {
        emptyOk = emptyOk && partyGetU16(saveGameRecord(&save, SaveSectionHeaderAndParty, i), PartyFieldClass) == 0;
    }
    check("slots 1-5 are empty", emptyOk);
    if (chapter == 2) {
        check("Chapter 2 opens at (166, 36) facing west", saveHeaderGetU16(&save, SaveHeaderWorldX) == 166 &&
                                                              saveHeaderGetU16(&save, SaveHeaderWorldY) == 36 &&
                                                              saveHeaderGetU16(&save, SaveHeaderFacing) == SaveFacingWest);
    } else {
        check("Chapter 3 opens at (460, 46) facing north", saveHeaderGetU16(&save, SaveHeaderWorldX) == 460 &&
                                                               saveHeaderGetU16(&save, SaveHeaderWorldY) == 46 &&
                                                               saveHeaderGetU16(&save, SaveHeaderFacing) == SaveFacingNorth);
    }
    const uint8_t *tpl = image + (chapter == 2 ? NewGameTemplateOffsetYendor2 : NewGameTemplateOffsetYendor3);
    check("the asserted opening values equal the template's own", saveHeaderGetU16(&save, SaveHeaderGameYear) == (uint16_t)(tpl[0xA0] | (tpl[0xA1] << 8)) &&
                                                                      saveHeaderGetU16(&save, SaveHeaderClockMinutes) == (uint16_t)(tpl[0xA2] | (tpl[0xA3] << 8)));
    free(image);
}

int main(void) {
    testShortImage();
    testSynthetic();
    testReal(2);
    testReal(3);

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

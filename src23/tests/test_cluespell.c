/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluespell test_cluespell.c ../cluespell.c ../cluebook.c ../exedata.c ../chargen.c ../font.c ../party.c ../spellrecord.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c ../monster.c ../lightsource.c && ./test_cluespell
 *
 * The real-data checks read yendor{2,3}/game/{SW.EXE,REGISTER.EXE} and WORLD.DAT (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cluespell.h"
#include "spellrecord.h"

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

static const uint8_t *pic(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[320 * 200];
    (void)ctx;
    memset(pixels, (uint8_t)(0x40 + category * 16 + id), sizeof(pixels));
    return pixels;
}

static bool anyColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (screen[y * 320 + x] == colour) {
                return true;
            }
        }
    }
    return false;
}

static void put16(uint8_t *m, unsigned offset, unsigned value) {
    m[offset] = (uint8_t)value;
    m[offset + 1] = (uint8_t)(value >> 8);
}

static void fillText(ClueSpellText *t) {
    memset(t, 0, sizeof(*t));
    strcpy(t->header, "CLASS:    LEVEL:");
    t->costLabelCount = 3;
    strcpy(t->costLabels[0], "MP:");
    strcpy(t->costLabels[1], "NUORE:");
    strcpy(t->costLabels[2], "ORE:");
    strcpy(t->effectLabels[0], "AFFECTS:");
    strcpy(t->effectLabels[2], "WHEN:");
    strcpy(t->effectLabels[4], "EFFECT:");
    const char *classes[6] = {"MONK", "ALCHEMIST", "PALADIN", "MAGE", "DRUID", "MARKSMAN"};
    for (unsigned i = 0; i < 6; i++) {
        strcpy(t->classNames[i], classes[i]);
    }
    strcpy(t->scroll, "SCROLL");
    strcpy(t->creation, "CREATION");
    strcpy(t->training, "TRAINING");
    strcpy(t->all, "ALL");
    strcpy(t->one, "ONE");
    strcpy(t->visibleMonsters, "VISIBLE MONSTERS");
    strcpy(t->visibleUndeads, "VISIBLE UNDEADS");
    strcpy(t->monster, "MONSTER");
    strcpy(t->insect, "INSECT");
    strcpy(t->undead, "UNDEAD");
    strcpy(t->character, "CHARACTER");
    strcpy(t->plural, "S");
    strcpy(t->handToHand, "IN HAND TO HAND");
    strcpy(t->straight, "IN A STRAIGHT LINE");
    strcpy(t->area, "IN A 3X3 AREA");
    strcpy(t->distance, "AT A DISTANCE");
    strcpy(t->outOfHand, "OUT OF HAND TO HAND");
    strcpy(t->anytime, "ANYTIME");
}

static void testDraw(void) {
    static ClueSpellText t;
    fillText(&t);
    uint8_t record[SpellRecordSize];
    memset(record, 0, sizeof(record));
    memcpy(record, "HEAL", 4);
    put16(record, SpellFieldMpCost, 7);
    put16(record, SpellFieldNuoreCost, 2);
    put16(record, SpellFieldOreCost, 1);
    put16(record, SpellFieldRequiredLevel, 5);
    put16(record, SpellFieldClassEligibility, 0x04); /* MAGE (the fourth: bit 0x20 >> 3) */
    put16(record, SpellFieldFlagsB, 0x0100);         /* a distance attack on one monster */
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, pic, NULL, screen, NULL};
    ClueSpellDescription d;
    memset(&d, 0, sizeof(d));
    uint8_t line[ClueSpellLineSize];
    memset(line, 0, sizeof(line));
    memcpy(line, "RESTORES HEALTH", 15);
    d.lines[0] = line;
    d.lineCount = 1;

    memset(screen, 0xEE, sizeof(screen));
    clueSpellPageDraw(&r, &t, record, 125, "SPELL INFORMATION", 0x8000, &d);
    check("backdrop, name and heading", screen[321] == 0x40 + 13 && anyColour(screen, 6, 4, 30, 10, 0x0D) && anyColour(screen, 213, 4, 316, 10, 0x0D));
    check("labels and the three costs (7, 2, 1) in 0x8A at x = 236", anyColour(screen, 44, 28, 120, 34, 0x0A) && anyColour(screen, 200, 40, 230, 46, 0x0A) &&
                                                                        anyColour(screen, 236, 40, 245, 46, 0x8A) && anyColour(screen, 236, 46, 245, 52, 0x8A) &&
                                                                        anyColour(screen, 236, 52, 245, 58, 0x8A));
    check("the spell is eligible only for the MAGE: one scroll row at y = 34 in 0xCA with the level 5", anyColour(screen, 122, 34, 160, 40, 0xCA) && anyColour(screen, 104, 34, 112, 40, 0x0D) &&
                                                                                                         !anyColour(screen, 122, 40, 160, 46, 0xCA) && !anyColour(screen, 122, 34, 160, 52, 0x8A));
    check("AFFECTS: ONE MONSTER ... AT A DISTANCE in 0xCA / 0xD on y = 76", anyColour(screen, 92, 76, 110, 82, 0xCA) && anyColour(screen, 116, 76, 160, 82, 0x0D) && anyColour(screen, 170, 76, 260, 82, 0xCA));
    check("WHEN: ANYTIME in 0xA7 at y = 88 and the description line at y = 106 in 0xD", anyColour(screen, 74, 88, 130, 94, 0xA7) && anyColour(screen, 44, 106, 120, 112, 0x0D));

    /* a starting ability of the MONK (class 4: abilities 1 and 3) */
    put16(record, SpellFieldClassEligibility, 0);
    memset(screen, 0xEE, sizeof(screen));
    clueSpellPageDraw(&r, &t, record, 3, "SPELL INFORMATION", 0x8000, NULL);
    check("spell 3 is a MONK starting ability: CREATION in 0xA7 at y = 34", anyColour(screen, 122, 34, 170, 40, 0xA7));

    /* flags B with the party bit: CHARACTER; with a low byte: the AFFECTS line is skipped */
    put16(record, SpellFieldFlagsB, 0x4000);
    memset(screen, 0xEE, sizeof(screen));
    clueSpellPageDraw(&r, &t, record, 125, "SPELL INFORMATION", 0x8000, NULL);
    check("flags B 0x4000: ALL CHARACTER", anyColour(screen, 116, 76, 170, 82, 0x0D));
    put16(record, SpellFieldFlagsB, 0x0001);
    memset(screen, 0xEE, sizeof(screen));
    clueSpellPageDraw(&r, &t, record, 125, "SPELL INFORMATION", 0x8000, NULL);
    check("a flags B low byte skips the AFFECTS value", !anyColour(screen, 92, 76, 300, 82, 0xCA));
}

static uint8_t *slurp(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)n);
    if (data && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return data;
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *exeName, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, exeName);
    size_t exeSize, worldSize;
    uint8_t *exeData = slurp(path, &exeSize);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    uint8_t *world = slurp(path, &worldSize);
    if (!exeData || !world) {
        printf("SKIP %s (game files not found)\n", label);
        g_skipCount++;
        free(exeData);
        free(world);
        return;
    }
    ExeData exe;
    static ClueSpellText t;
    check(label, exeDataOpen(&exe, game, exeData, exeSize) && clueSpellTextLoad(&t, &exe, game) && strncmp(t.classNames[5], "MARKSMAN", 8) == 0 &&
                     t.costLabelCount == (game == GameYendor2 ? 3u : 2u) && strcmp(t.anytime, "ANYTIME") == 0);
    unsigned with = 0, total = game == GameYendor2 ? 124 : 107;
    bool linesOk = true;
    for (unsigned id = 1; id <= total; id++) {
        unsigned first, count;
        if (!spellDescriptionRange(game, world, worldSize, id, &first, &count)) {
            linesOk = false;
            continue;
        }
        if (count > 0 && count < 16) {
            with++;
            for (unsigned i = 0; i < count; i++) {
                const uint8_t *line = spellDescriptionLine(game, world, worldSize, first + i);
                linesOk = linesOk && line != NULL && line[0] < 0x7F;
            }
        }
    }
    check("...most spells have a short description (Chapter 2 stops after 105)", linesOk && with * 10 > total * 8); /* Chapter 2 describes spells 1-105 of 124 */
    free(exeData);
    free(world);
}

int main(void) {
    testDraw();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: the spell page labels load from SW.EXE");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: the spell page labels load from REGISTER.EXE");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

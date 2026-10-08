/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_clueentries test_clueentries.c ../clueentries.c ../exedata.c && ./test_clueentries
 *
 * Synthetic data segment plus the real executables of yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clueentries.h"

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

static void put16(uint8_t *p, unsigned v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void testSynthetic(void) {
    static uint8_t exe[0x21660 + 0x10000];
    memset(exe, 0, sizeof(exe));
    uint8_t *ds = exe + 0x21660;
    put16(ds + 0xF3F4 + 2 * 1, 0xE000); /* category 2 */
    put16(ds + 0xE000, 2);
    put16(ds + 0xE002, 7);
    put16(ds + 0xE004, 0);
    put16(ds + 0xE006, 9);
    put16(ds + 0xE008, 0x8000);
    memcpy(ds + 0x8F04, "ONE\0TWO\0THREE\0", 14);
    ExeData data = {exe, sizeof(exe), 0x21660};
    ClueEntry entries[ClueEntriesMax];
    check("a list is a count and 4-byte entries", clueEntries(&data, GameYendor2, 2, entries) == 2 && entries[0].id == 7 && !entries[0].free && entries[1].id == 9 && entries[1].free);
    check("a category without a list has none", clueEntries(&data, GameYendor2, 3, entries) == 0 && clueEntries(&data, GameYendor2, 18, entries) == 0 && clueEntries(&data, GameYendor2, 0, entries) == 0);
    char text[ClueEntryTextSize];
    check("the packed string tables are read in order", clueEntryPackedString(&data, GameYendor2, 4, 3, text) && strcmp(text, "THREE") == 0 && clueEntryPackedString(&data, GameYendor2, 4, 1, text) && strcmp(text, "ONE") == 0);
    check("only categories 4 and 11 have packed strings", !clueEntryPackedString(&data, GameYendor2, 2, 1, text) && !clueEntryPackedString(&data, GameYendor2, 4, 0, text));
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *exeName, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, exeName);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    static uint8_t bytes[400000];
    size_t size = fread(bytes, 1, sizeof(bytes), f);
    fclose(f);
    ExeData exe;
    char text[160];
    if (!exeDataOpen(&exe, game, bytes, size)) {
        check("the executable opens", false);
        return;
    }
    ClueEntry entries[ClueEntriesMax];
    static const unsigned kCounts2[17] = {55, 61, 105, 6, 52, 50, 32, 47, 47, 31, 8, 41, 52, 43, 13, 27, 24};
    static const unsigned kCounts3[17] = {54, 71, 98, 6, 47, 46, 36, 43, 44, 31, 8, 38, 51, 26, 14, 8, 33};
    const unsigned *expected = game == GameYendor2 ? kCounts2 : kCounts3;
    bool counts = true;
    for (unsigned category = 1; category <= 17; category++) {
        counts = counts && clueEntries(&exe, game, category, entries) == expected[category - 1];
    }
    snprintf(text, sizeof(text), "%s: the 17 lists have the sizes read from the data segment", label);
    check(text, counts);
    unsigned open = 0;
    clueEntries(&exe, game, 4, entries);
    for (unsigned i = 0; i < 6; i++) {
        open += entries[i].free;
    }
    check("the six class entries are 1-6 and all open in the shareware", entries[0].id == 1 && entries[5].id == 6 && open == 6);
    char name[ClueEntryTextSize];
    check("the class names are the three tiers", clueEntryPackedString(&exe, game, 4, 1, name) && strncmp(name, "MONK / ", 7) == 0 && clueEntryPackedString(&exe, game, 4, 6, name) && strncmp(name, "MARKSMAN", 8) == 0);
    check("the item groups: armor first, weapons eighth", clueEntryPackedString(&exe, game, 11, 1, name) && strncmp(name, "ARMOR", 5) == 0 && clueEntryPackedString(&exe, game, 11, 8, name) && strcmp(name, "WEAPONS") == 0);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

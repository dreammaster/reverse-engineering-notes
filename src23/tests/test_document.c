/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_document test_document.c ../document.c ../document_stdio.c && ./test_document
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "document.h"
#include "document_stdio.h"

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

static DocumentCatalog g_catalog;
static uint8_t g_region[DocumentRegionSizeMax];

static void testLayouts(void) {
    checkU32("line widths: Journal 16", DocumentLineWidth[DocumentCategoryJournal], 16);
    checkU32("line widths: Note 23", DocumentLineWidth[DocumentCategoryNote], 23);
    checkU32("line widths: Document 22", DocumentLineWidth[DocumentCategoryDocument], 22);
    checkU32("line widths: Unused 23", DocumentLineWidth[DocumentCategoryUnused], 23);

    checkU32("yendor2 region starts at 0x15181C", documentCatalogLayout(GameYendor2)->regionOffset, 0x15181C);
    checkU32("yendor2 region is 14332 bytes", documentCatalogLayout(GameYendor2)->regionSize, 14332);
    checkU32("yendor3 region starts at 0x3C2030", documentCatalogLayout(GameYendor3)->regionOffset, 0x3C2030);
    checkU32("yendor3 region is 11986 bytes", documentCatalogLayout(GameYendor3)->regionSize, 11986);

    checkU32("yendor2 Journal has 6 entries", documentCount(GameYendor2, DocumentCategoryJournal), 6);
    checkU32("yendor2 Note has 1 (empty) entry", documentCount(GameYendor2, DocumentCategoryNote), 1);
    checkU32("yendor2 Document has 26 entries", documentCount(GameYendor2, DocumentCategoryDocument), 26);
    checkU32("yendor3 Journal has 8 entries", documentCount(GameYendor3, DocumentCategoryJournal), 8);
    checkU32("yendor3 Note has 8 entries", documentCount(GameYendor3, DocumentCategoryNote), 8);
}

/*
 * The four categories' entries, taken together, must exactly tile the
 * game's whole text region with no gaps or overlaps: summing every
 * entry's byte span (lines * line width) across all categories should
 * equal regionSize, since Journal/Note/Document/Unused were found to
 * share one contiguous span (see document.h).
 */
static void checkTableInvariants(GameKind game) {
    char label[96];
    uint32_t totalBytes = 0;
    for (int c = 0; c < DocumentCategoryCount; c++) {
        unsigned count = documentCount(game, (DocumentCategory)c);
        for (unsigned id = 1; id <= count; id++) {
            totalBytes += documentLineCount(game, (DocumentCategory)c, id) * (uint32_t)DocumentLineWidth[c];
        }
    }
    snprintf(label, sizeof(label), "%s: every category's entries together exactly tile the text region",
             game == GameYendor2 ? "yendor2" : "yendor3");
    checkU32(label, totalBytes, documentCatalogLayout(game)->regionSize);
}

static void testParseAndBounds(void) {
    const DocumentCatalogLayout *layout = documentCatalogLayout(GameYendor2);
    for (uint32_t i = 0; i < layout->regionSize; i++) {
        g_region[i] = (uint8_t)(i * 11 + 5);
    }
    check("a region one byte short is rejected", !documentCatalogParse(&g_catalog, GameYendor2, g_region, layout->regionSize - 1));
    check("an exact region parses", documentCatalogParse(&g_catalog, GameYendor2, g_region, layout->regionSize));

    check("Journal id 0 has 0 lines", documentLineCount(GameYendor2, DocumentCategoryJournal, 0) == 0);
    check("Journal id 7 (past count) has 0 lines", documentLineCount(GameYendor2, DocumentCategoryJournal, 7) == 0);
    checkU32("Journal id 1 has 78 lines", documentLineCount(GameYendor2, DocumentCategoryJournal, 1), 78);
    checkU32("Journal id 6 has 26 lines", documentLineCount(GameYendor2, DocumentCategoryJournal, 6), 26);
    check("yendor2 Note id 1 is empty (0 lines)", documentLineCount(GameYendor2, DocumentCategoryNote, 1) == 0);

    char line[DocumentLineBufferSize];
    check("line 0 of Journal id 1 reads from the region's own first bytes, stopping at the pattern's first zero byte",
          documentGetLine(&g_catalog, DocumentCategoryJournal, 1, 0, line) &&
              memcmp(line, g_region, strlen(line)) == 0 && strlen(line) < DocumentLineWidth[DocumentCategoryJournal]);
    check("line 78 of Journal id 1 is out of range (only 78 lines, 0-77)",
          !documentGetLine(&g_catalog, DocumentCategoryJournal, 1, 78, line));
    check("Note id 1's only line (0 of 0) is out of range", !documentGetLine(&g_catalog, DocumentCategoryNote, 1, 0, line));

    static uint8_t world[0x15181C + DocumentRegionSizeYendor2 + 16];
    memcpy(world + layout->regionOffset, g_region, layout->regionSize);
    check("whole-file parse finds the region",
          documentCatalogParseWorldDat(&g_catalog, GameYendor2, world, sizeof(world)) &&
              documentGetLine(&g_catalog, DocumentCategoryJournal, 1, 0, line));
    check("a WORLD.DAT shorter than the region is rejected",
          !documentCatalogParseWorldDat(&g_catalog, GameYendor2, world, layout->regionOffset + layout->regionSize - 1));
}

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return documentCatalogReadWorldDatFile(&g_catalog, game, path);
}

static bool lineIs(DocumentCategory category, unsigned id, unsigned lineIndex, const char *expected) {
    char line[DocumentLineBufferSize];
    return documentGetLine(&g_catalog, category, id, lineIndex, line) && strcmp(line, expected) == 0;
}

/* Every real line must be printable ASCII (or the game's `~`/backtick quote-glyph substitutes) and end-to-end readable. */
static void checkRealInvariants(const char *name, GameKind game) {
    char label[96];
    bool printable = true;
    bool readable = true;
    unsigned totalLines = 0;

    for (int c = 0; c < DocumentCategoryCount; c++) {
        unsigned count = documentCount(game, (DocumentCategory)c);
        for (unsigned id = 1; id <= count; id++) {
            unsigned lines = documentLineCount(game, (DocumentCategory)c, id);
            for (unsigned l = 0; l < lines; l++) {
                char line[DocumentLineBufferSize];
                if (!documentGetLine(&g_catalog, (DocumentCategory)c, id, l, line)) {
                    readable = false;
                    continue;
                }
                totalLines++;
                for (const char *p = line; *p; p++) {
                    if ((unsigned char)*p < ' ' || (unsigned char)*p > '~') {
                        printable = false;
                    }
                }
            }
            char overLine[DocumentLineBufferSize];
            if (documentGetLine(&g_catalog, (DocumentCategory)c, id, lines, overLine)) {
                readable = false; /* one past the real line count must fail */
            }
        }
    }

    snprintf(label, sizeof(label), "%s: every real line is fully readable (in range) and no line reads past its count", name);
    check(label, readable);
    snprintf(label, sizeof(label), "%s: every real line is printable ASCII", name);
    check(label, printable);
    snprintf(label, sizeof(label), "%s: at least 100 real lines were read", name);
    check(label, totalLines >= 100);
}

static void testRealYendor2(void) {
    if (!loadReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game")) {
        printf("SKIP yendor2 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    check("Journal id 1 line 0 is a dated history entry", lineIs(DocumentCategoryJournal, 1, 0, "06/17/519      "));
    check("Journal id 1 line 2 names the paltivar exile",
          lineIs(DocumentCategoryJournal, 1, 2, "TODAY, THE     "));
    check("Journal id 6 (undated verse) line 0", lineIs(DocumentCategoryJournal, 6, 0, "THE FOREST IS  "));

    check("Document id 1 is the guild puzzle", lineIs(DocumentCategoryDocument, 1, 0, "-PUZZLE OF THE GUILD-"));
    check("Document id 1's key sequence line", lineIs(DocumentCategoryDocument, 1, 10, "1-2-3-4-5-1-4-2-5-3-1"));
    check("Document id 2 is the Order of the Opposition manifesto",
          lineIs(DocumentCategoryDocument, 2, 0, "    \"ORDER OF THE    "));

    checkRealInvariants("yendor2", GameYendor2);
}

static void testRealYendor3(void) {
    if (!loadReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game")) {
        printf("SKIP yendor3 real-data checks (no WORLD.DAT)\n");
        g_skipCount++;
        return;
    }

    check("Journal id 1 line 0 is a dated history entry", lineIs(DocumentCategoryJournal, 1, 0, "03/15/547      "));
    check("Note id 3 line 12 contains a spoken password",
          lineIs(DocumentCategoryNote, 3, 12, "THE PASSWORD IS `RUSE~"));
    check("Note id 3's last line signs off as Queen Obversia",
          lineIs(DocumentCategoryNote, 3, 15, "   -QUEEN OBVERSIA    "));
    check("Document id 1 is titled and signed", lineIs(DocumentCategoryDocument, 1, 0, "   %THE BLACK CAT%   "));
    check("Document id 1's byline", lineIs(DocumentCategoryDocument, 1, 2, "      BY JASPER      "));
    check("Document id 7 onward are empty (only 6 real entries)", documentLineCount(GameYendor3, DocumentCategoryDocument, 7) == 0);

    checkRealInvariants("yendor3", GameYendor3);
}

int main(void) {
    testLayouts();
    checkTableInvariants(GameYendor2);
    checkTableInvariants(GameYendor3);
    testParseAndBounds();
    testRealYendor2();
    testRealYendor3();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

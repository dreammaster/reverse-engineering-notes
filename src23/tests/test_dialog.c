/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_dialog test_dialog.c ../dialog.c ../globalflags.c && ./test_dialog
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game
 * (override with YENDOR2_GAME_DIR / YENDOR3_GAME_DIR) and are skipped if absent.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "globalflags.h"

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void testLayouts(void) {
    const DialogLayout *y2 = dialogLayout(GameYendor2);
    const DialogLayout *y3 = dialogLayout(GameYendor3);
    check("both games have a layout", y2 && y3);
    checkU32("Chapter 2 headers precede topics by 150 slots (6000 bytes)", y2->topicOffset - y2->npcOffset, 150 * 40);
    checkU32("Chapter 3 headers end exactly where topics begin", y3->topicOffset - y3->npcOffset,
             (uint32_t)DialogNpcCountYendor3 * DialogNpcRecordSize);
    checkU32("Chapter 2 topics (58 bytes) end where lines begin, with 72 entries of slack", y2->lineOffset - y2->topicOffset,
             (uint32_t)(DialogTopicCountYendor2 + 72) * y2->topicRecordSize);
    checkU32("Chapter 3 topics (60 bytes) end exactly where lines begin", y3->lineOffset - y3->topicOffset,
             (uint32_t)DialogTopicCountYendor3 * y3->topicRecordSize);
}

static void putU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)(value & 0xFF);
    record[offset + 1] = (uint8_t)(value >> 8);
}

static void testOpeningTopic(void) {
    uint8_t npc[DialogNpcRecordSize];
    memset(npc, 0, sizeof(npc));
    putU16(npc, DialogNpcFirstTopic, 100);
    checkU32("no flag: the first topic", dialogOpeningTopic(npc, false, false, false), 100);
    checkU32("flag A: the second", dialogOpeningTopic(npc, true, false, false), 101);
    checkU32("flag B: the third", dialogOpeningTopic(npc, false, true, false), 102);
    checkU32("flag C: the fourth", dialogOpeningTopic(npc, false, false, true), 103);
    checkU32("A outranks B and C", dialogOpeningTopic(npc, true, true, true), 101);
    checkU32("B outranks C", dialogOpeningTopic(npc, false, true, true), 102);
}

static void fillLine(DialogCatalog *catalog, unsigned index, const char *text) {
    uint8_t *line = catalog->lines + (size_t)index * DialogLineSize;
    memset(line, ' ', DialogLineChars);
    memcpy(line, text, strlen(text));
    line[DialogLineChars] = 0;
}

static void testTopicTextAssembly(void) {
    static DialogCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.lineCount = 10;
    fillLine(&catalog, 4, "FIRST LINE.%");
    fillLine(&catalog, 5, "SECOND LINE.   ");
    fillLine(&catalog, 6, "THIRD.");

    uint8_t npc[DialogNpcRecordSize], topic[DialogTopicRecordSizeMax];
    memset(npc, 0, sizeof(npc));
    memset(topic, 0, sizeof(topic));
    putU16(npc, DialogNpcFirstLine, 3);
    putU16(topic, DialogTopicTextOffset, 34); /* one line into the NPC's block: absolute line 4 */
    putU16(topic, DialogTopicTextLines, 3);

    char out[200];
    size_t n = dialogTopicText(&catalog, npc, topic, out, sizeof(out));
    char expected[200];
    snprintf(expected, sizeof(expected), "%-33s%-33s%s", "FIRST LINE.%", "SECOND LINE.", "THIRD.");
    check("lines are concatenated with their padding, trailing spaces trimmed", strcmp(out, expected) == 0);
    checkU32("the returned length is the text length", (uint32_t)n, (uint32_t)strlen(out));
    check("it ends at the last real character", out[n - 1] == '.');

    char small[8];
    size_t full = dialogTopicText(&catalog, npc, topic, small, sizeof(small));
    checkU32("a short buffer still reports the full length", (uint32_t)full, (uint32_t)n);
    checkU32("...and is NUL terminated", small[7], 0);

    putU16(topic, DialogTopicTextLines, 0);
    checkU32("zero lines is the empty string", (uint32_t)dialogTopicText(&catalog, npc, topic, out, sizeof(out)), 0);
    checkU32("...with a terminator", out[0], 0);
}

/* Space-separated names of the NPC's listed topics. */
static void listedNames(const DialogCatalog *catalog, const uint8_t *npc, const DialogState *state, const uint8_t *flags,
                        char *out, size_t capacity) {
    out[0] = '\0';
    for (unsigned t = 0; t < dialogGetU16(npc, DialogNpcTopicCount); t++) {
        const uint8_t *topic = dialogTopic(catalog, dialogGetU16(npc, DialogNpcFirstTopic) + t);
        if (dialogTopicListed(catalog, topic, state, flags, 512)) {
            char name[DialogTopicNameSize + 1];
            dialogTopicName(topic, name);
            if (out[0]) {
                strncat(out, " ", capacity - strlen(out) - 1);
            }
            strncat(out, name, capacity - strlen(out) - 1);
        }
    }
}

static void testMasksSynthetic(void) {
    static DialogCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.topicRecordSize = DialogTopicRecordSizeYendor2;
    uint8_t flags[512];
    memset(flags, 0, sizeof(flags));
    uint8_t topic[DialogTopicRecordSizeMax];
    memset(topic, 0, sizeof(topic));
    DialogState state;
    memset(&state, 0, sizeof(state));

    check("a topic with no own bits is never listed", !dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    putU16(topic, DialogTopicOwnMaskA, 0x0004);
    check("an own bit that isn't available isn't listed", !dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    state.availA = 0x0006;
    check("...and is once the masks overlap", dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    putU16(topic, DialogTopicOwnMaskB, 0x0001);
    check("a nonzero B mask must overlap too", !dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    state.availB = 0x0001;
    check("both overlapping lists it", dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));

    putU16(topic, DialogTopicRequireFirst, 5);
    check("a positive require-flag that is clear hides it", !dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    globalFlagSet(flags, sizeof(flags), 5);
    check("...set, it shows", dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    putU16(topic, DialogTopicRequireFirst + 2, (uint16_t)-7);
    check("a negative require-flag that is clear passes", dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));
    globalFlagSet(flags, sizeof(flags), 7);
    check("...but hides it once set", !dialogTopicListed(&catalog, topic, &state, flags, sizeof(flags)));

    /* visiting */
    uint8_t visit[DialogTopicRecordSizeMax];
    memset(visit, 0, sizeof(visit));
    memset(&state, 0, sizeof(state));
    state.availA = 0x00F0;
    state.availB = 0x0003;
    putU16(visit, DialogTopicOwnMaskA, 0x0010);
    putU16(visit, DialogTopicClearMaskA, 0x0010);
    putU16(visit, DialogTopicUnlockMaskA, 0x0100);
    putU16(visit, DialogTopicClearMaskB, 0x0001);
    putU16(visit, DialogTopicUnlockMaskB, 0x0008);
    putU16(visit, DialogTopicResultFirst, 20);
    putU16(visit, DialogTopicResultFirst + 2, (uint16_t)-5);
    uint8_t flags2[512];
    memset(flags2, 0, sizeof(flags2));
    globalFlagSet(flags2, sizeof(flags2), 5);
    dialogVisitTopic(&catalog, visit, &state, flags2, sizeof(flags2), true);
    checkU32("visiting clears then unlocks mask A", state.availA, 0x01E0);
    checkU32("...and mask B", state.availB, 0x000A);
    check("a positive result flag sets", globalFlagTest(flags2, sizeof(flags2), 20));
    check("a negative result flag clears", !globalFlagTest(flags2, sizeof(flags2), 5));

    /* conditional results */
    memset(flags2, 0, sizeof(flags2));
    putU16(visit, DialogTopicFlags, DialogTopicConditional);
    dialogVisitTopic(&catalog, visit, &state, flags2, sizeof(flags2), false);
    check("a conditional topic's flags wait for success", !globalFlagTest(flags2, sizeof(flags2), 20));
    dialogVisitTopic(&catalog, visit, &state, flags2, sizeof(flags2), true);
    check("...and apply with it", globalFlagTest(flags2, sizeof(flags2), 20));

    /* save / restore the menu */
    memset(&state, 0, sizeof(state));
    state.availA = 0x0C10;
    state.availB = 0x0001;
    uint8_t save[DialogTopicRecordSizeMax], restore[DialogTopicRecordSizeMax];
    memset(save, 0, sizeof(save));
    memset(restore, 0, sizeof(restore));
    putU16(save, DialogTopicFlags, DialogTopicSaveMenu);
    putU16(save, DialogTopicOwnMaskA, 0x0010);
    putU16(save, DialogTopicClearMaskA, 0xFFFF);
    putU16(save, DialogTopicUnlockMaskA, 0x3000);
    putU16(restore, DialogTopicFlags, DialogTopicRestoreMenu);
    dialogVisitTopic(&catalog, save, &state, flags2, sizeof(flags2), true);
    checkU32("a save topic leaves its own new menu", state.availA, 0x3000);
    checkU32("...having saved the old masks minus its own bits", state.savedA, 0x0C00);
    dialogVisitTopic(&catalog, restore, &state, flags2, sizeof(flags2), true);
    checkU32("a restore topic brings the saved menu back", state.availA, 0x0C00);
    checkU32("...including mask B", state.availB, state.savedB);
}

static void testRealConversations(const DialogCatalog *catalog, GameKind game) {
    uint8_t flags[512];
    memset(flags, 0, sizeof(flags));
    char names[200];
    DialogState state;
    memset(&state, 0, sizeof(state));
    const uint8_t *npc = dialogNpc(catalog, 1);
    dialogVisitTopic(catalog, dialogTopic(catalog, dialogGetU16(npc, DialogNpcFirstTopic)), &state, flags, sizeof(flags), true);
    listedNames(catalog, npc, &state, flags, names, sizeof(names));
    if (game == GameYendor2) {
        check("Chapter 2 governor, after HELLO: BLACKWING, PORT HOPE and BYE", strcmp(names, "BLACKWING PORT HOPE BYE") == 0);
        npc = dialogNpc(catalog, 6);
        memset(&state, 0, sizeof(state));
        unsigned first = dialogGetU16(npc, DialogNpcFirstTopic);
        dialogVisitTopic(catalog, dialogTopic(catalog, first), &state, flags, sizeof(flags), true);
        listedNames(catalog, npc, &state, flags, names, sizeof(names));
        check("the tavern greets with PURCHASE FOOD, GO TO MENU, FINISHED", strcmp(names, "PURCHASE FOOD GO TO MENU FINISHED") == 0);
        dialogVisitTopic(catalog, dialogTopic(catalog, first + 3), &state, flags, sizeof(flags), true); /* GO TO MENU */
        listedNames(catalog, npc, &state, flags, names, sizeof(names));
        check("GO TO MENU offers YES / NO", strcmp(names, "YES NO") == 0);
        dialogVisitTopic(catalog, dialogTopic(catalog, first + 5), &state, flags, sizeof(flags), true); /* NO */
        listedNames(catalog, npc, &state, flags, names, sizeof(names));
        check("NO restores the original menu", strcmp(names, "PURCHASE FOOD GO TO MENU FINISHED") == 0);
    } else {
        check("Chapter 3 NPC 1, after HELLO: NAME, ZAMORA, TASKS, BYE", strcmp(names, "NAME ZAMORA TASKS BYE") == 0);
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
    uint8_t *data = malloc((size_t)len);
    if (data && fread(data, 1, (size_t)len, f) != (size_t)len) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return data;
}

static void checkRealGame(const char *name, GameKind game, const char *envName, const char *fallbackDir) {
    char path[512], label[160];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    size_t size;
    uint8_t *image = loadFile(path, &size);
    if (!image) {
        printf("SKIP %s real-data checks (no WORLD.DAT)\n", name);
        g_skipCount++;
        return;
    }
    static DialogCatalog catalog;
    snprintf(label, sizeof(label), "%s: the catalog parses", name);
    check(label, dialogCatalogParseWorldDat(&catalog, game, image, size));
    free(image);

    /* the NPCs tile the topic and line tables exactly */
    unsigned expectedTopic = 1, expectedLine = 1;
    bool tiled = true, ranges = true, names = true, lines = true;
    unsigned npcs = 0;
    for (unsigned id = 1; id < catalog.npcCount; id++) {
        const uint8_t *npc = dialogNpc(&catalog, id);
        unsigned topics = dialogGetU16(npc, DialogNpcTopicCount);
        unsigned lineCount = dialogGetU16(npc, DialogNpcLineCount);
        if (topics == 0 && lineCount == 0) {
            continue; /* not a real NPC */
        }
        npcs++;
        if (dialogGetU16(npc, DialogNpcFirstTopic) != expectedTopic || dialogGetU16(npc, DialogNpcFirstLine) != expectedLine) {
            tiled = false;
        }
        expectedTopic += topics;
        expectedLine += lineCount;
        for (unsigned t = 0; t < topics; t++) {
            const uint8_t *topic = dialogTopic(&catalog, dialogGetU16(npc, DialogNpcFirstTopic) + t);
            char topicName[DialogTopicNameSize + 1];
            dialogTopicName(topic, topicName);
            if (topicName[0] == '\0') {
                names = false;
            }
            for (unsigned c = 0; topicName[c]; c++) {
                if (topicName[c] < ' ' || topicName[c] > '~') {
                    names = false;
                }
            }
            unsigned offset = dialogTopicU16(&catalog, topic, DialogTopicTextOffset);
            unsigned count = dialogTopicU16(&catalog, topic, DialogTopicTextLines);
            /* a topic without text keeps something else in these words (a bit mask) */
            if (count != 0 && (offset % DialogLineSize != 0 || offset / DialogLineSize + count > lineCount)) {
                ranges = false;
            }
        }
    }
    snprintf(label, sizeof(label), "%s: header chain tiles the topic table exactly", name);
    checkU32(label, expectedTopic, catalog.topicCount);
    snprintf(label, sizeof(label), "%s: header chain tiles the line table exactly", name);
    checkU32(label, expectedLine, catalog.lineCount);
    snprintf(label, sizeof(label), "%s: every header starts where the previous one ended", name);
    check(label, tiled);
    snprintf(label, sizeof(label), "%s: every topic has a printable, non-empty name", name);
    check(label, names);
    snprintf(label, sizeof(label), "%s: every topic's text lies inside its NPC's line block", name);
    check(label, ranges);
    snprintf(label, sizeof(label), "%s: more than 100 NPCs", name);
    check(label, npcs > 100);

    for (unsigned i = 1; i < catalog.lineCount; i++) {
        const uint8_t *line = dialogLine(&catalog, i);
        if (line[DialogLineChars] != 0) {
            lines = false;
        }
        for (unsigned c = 0; c < DialogLineChars; c++) {
            if (line[c] < ' ' || line[c] > '~') {
                lines = false;
            }
        }
    }
    snprintf(label, sizeof(label), "%s: every text line is printable ASCII with a NUL at 33", name);
    check(label, lines);

    testRealConversations(&catalog, game);

    if (game == GameYendor2) {
        const uint8_t *governor = dialogNpc(&catalog, 1);
        const uint8_t *hello = dialogTopic(&catalog, dialogGetU16(governor, DialogNpcFirstTopic));
        char text[512], topicName[DialogTopicNameSize + 1];
        dialogTopicName(hello, topicName);
        check("Chapter 2 NPC 1's first topic is HELLO", strcmp(topicName, "HELLO") == 0);
        dialogTopicText(&catalog, governor, hello, text, sizeof(text));
        check("...and its text is the governor's greeting", strncmp(text, "AS YOU APPROACH, YOU CAN SEE THAT", 33) == 0);
        checkU32("NPC 1 has 15 topics", dialogGetU16(governor, DialogNpcTopicCount), 15);
        checkU32("...and 90 lines", dialogGetU16(governor, DialogNpcLineCount), 90);
        checkU32("...and the portrait picture 185", dialogGetU16(governor, DialogNpcPictureId), 185);
        const uint8_t *tome = dialogNpc(&catalog, 43);
        check("Chapter 2 NPC 43 is a +10 Strength tome (flag 142, offset 0x3C)",
              dialogGetU16(tome, DialogNpcOneTimeFlag) == 142 && dialogGetU16(tome, DialogNpcParamA) == 0x3C &&
                  dialogGetU16(tome, DialogNpcParamB) == 10);
        const uint8_t *bye = dialogTopic(&catalog, dialogGetU16(governor, DialogNpcFirstTopic) + 14);
        dialogTopicName(bye, topicName);
        check("its last topic is BYE and ends the conversation",
              strcmp(topicName, "BYE") == 0 && (dialogGetU16(bye, DialogTopicFlags) & DialogTopicEndsConversation));
    }
    if (game == GameYendor3) {
        const uint8_t *npc = dialogNpc(&catalog, 1);
        static const char *expected[] = {"HELLO", "HELLO 2", "NAME", "ZAMORA", "TASKS"};
        bool namesOk = true;
        for (unsigned i = 0; i < 5; i++) {
            char topicName[DialogTopicNameSize + 1];
            dialogTopicName(dialogTopic(&catalog, dialogGetU16(npc, DialogNpcFirstTopic) + i), topicName);
            if (strcmp(topicName, expected[i]) != 0) {
                namesOk = false;
            }
        }
        check("Chapter 3 NPC 1's first topics are HELLO, HELLO 2, NAME, ZAMORA, TASKS (60-byte records)", namesOk);
        checkU32("NPC 1 has 26 topics", dialogGetU16(npc, DialogNpcTopicCount), 26);
        checkU32("...and 152 lines", dialogGetU16(npc, DialogNpcLineCount), 152);
        const uint8_t *hello = dialogTopic(&catalog, dialogGetU16(npc, DialogNpcFirstTopic));
        checkU32("HELLO shows 6 lines (read through the shifted field)", dialogTopicU16(&catalog, hello, DialogTopicTextLines), 6);
        const uint8_t *yes = dialogTopic(&catalog, dialogGetU16(npc, DialogNpcFirstTopic) + 5);
        check("a Chapter 3 topic's flags word is where Chapter 2's is", dialogGetU16(yes, DialogTopicFlags) == 16);
    }
}

int main(void) {
    testLayouts();
    testOpeningTopic();
    testTopicTextAssembly();
    testMasksSynthetic();
    checkRealGame("yendor2", GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game");
    checkRealGame("yendor3", GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

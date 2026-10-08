/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_conversation test_conversation.c ../conversation.c ../dialog.c ../globalflags.c && ./test_conversation
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conversation.h"
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

enum { FlagBytes = 96 };

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
    static DialogCatalog catalog;
    if (!dialogCatalogParseWorldDat(&catalog, game, world, size)) {
        check("the dialog catalog parses", false);
        return;
    }
    unsigned npcs = 0, started = 0, endedByEscape = 0, endedRandom = 0, deadEnds = 0, speakers = 0, conditionalSeen = 0, longest = 0;
    unsigned seed = 12345;
    for (unsigned id = 1; id < catalog.npcCount; id++) {
        if (!dialogNpc(&catalog, id) || dialogGetU16(dialogNpc(&catalog, id), DialogNpcTopicCount) == 0) {
            continue;
        }
        npcs++;
        uint8_t flags[FlagBytes];

        /* Escape every time: the last topic listed should be a way out */
        memset(flags, 0, sizeof(flags));
        Conversation conv;
        ConversationStep step;
        if (!conversationStart(&conv, &catalog, id, flags, sizeof(flags), false, &step)) {
            continue;
        }
        started++;
        speakers += conversationNeedsSpeaker(conv.npc);
        unsigned steps = 0;
        while (!conv.ended && conv.listedCount > 0 && steps < 60) {
            step = conversationChoose(&conv, 0, flags, sizeof(flags));
            if (step.needsResolve) {
                conversationResolve(&conv, true, flags, sizeof(flags));
            }
            steps++;
        }
        endedByEscape += conv.ended;

        /* random topics, conditional ones succeeding or failing at random */
        memset(flags, 0, sizeof(flags));
        conversationStart(&conv, &catalog, id, flags, sizeof(flags), true, &step);
        steps = 0;
        while (!conv.ended && conv.listedCount > 0 && steps < 300) {
            seed = seed * 1103515245u + 12345u;
            step = conversationChoose(&conv, 1 + (seed >> 16) % conv.listedCount, flags, sizeof(flags));
            if (step.needsResolve) {
                conditionalSeen++;
                seed = seed * 1103515245u + 12345u;
                conversationResolve(&conv, (seed >> 16) & 1, flags, sizeof(flags));
            }
            steps++;
        }
        endedRandom += conv.ended;
        deadEnds += !conv.ended && conv.listedCount == 0;
        if (steps > longest) {
            longest = steps;
        }
    }
    char text[160];
    snprintf(text, sizeof(text), "%s: every NPC with topics starts a conversation (%u of them)", label, npcs);
    check(text, npcs > 50 && started == npcs);
    snprintf(text, sizeof(text), "%s: pressing Escape at every list ends %u of %u conversations", label, endedByEscape, started);
    check(text, endedByEscape * 10 >= started * 9);
    snprintf(text, sizeof(text), "%s: random choices end %u, run into %u dead ends, %u conditional topics seen, longest %u steps, %u NPCs ask for a speaker", label, endedRandom, deadEnds,
             conditionalSeen, longest, speakers);
    check(text, endedRandom + deadEnds + 10 >= started && deadEnds * 10 <= started);
    free(world);
}

static void testBookkeeping(void) {
    /* a hand-made one-NPC catalog: greeting (opens topic 2 and lists it), a plain topic that closes with exit code 2 */
    static DialogCatalog c;
    memset(&c, 0, sizeof(c));
    c.game = GameYendor2;
    c.npcCount = 2;
    c.topicCount = 3;
    c.lineCount = 3;
    c.topicRecordSize = DialogTopicRecordSizeYendor2;
    uint8_t *npc = c.npcs + DialogNpcRecordSize;
    npc[DialogNpcTopicCount] = 3;
    npc[DialogNpcFirstTopic] = 0;
    npc[DialogNpcLineCount] = 3;
    uint8_t *greeting = c.topics, *second = c.topics + c.topicRecordSize, *bye = c.topics + 2 * c.topicRecordSize;
    memcpy(greeting, "HELLO        ", 13);
    greeting[DialogTopicOwnMaskA] = 0;
    greeting[DialogTopicUnlockMaskA] = 0x03; /* unlocks the other two */
    memcpy(second, "ASK          ", 13);
    second[DialogTopicOwnMaskA] = 0x01;
    second[DialogTopicFlags] = DialogTopicConditional;
    second[DialogTopicResultFirst] = 7; /* sets flag 7 on success */
    memcpy(bye, "BYE          ", 13);
    bye[DialogTopicOwnMaskA] = 0x02;
    bye[DialogTopicFlags] = DialogTopicEndsConversation;
    bye[DialogTopicArg] = 2;
    uint8_t flags[FlagBytes];
    memset(flags, 0, sizeof(flags));
    Conversation conv;
    ConversationStep step;
    check("a hand-made conversation starts", conversationStart(&conv, &c, 1, flags, sizeof(flags), false, &step) && step.topic == 0 && !conv.ended);
    check("the greeting's unlock masks list the two other topics", conv.listedCount == 2 && conv.listed[0] == 1 && conv.listed[1] == 2);
    step = conversationChoose(&conv, 1, flags, sizeof(flags));
    check("a conditional topic waits for its service before setting its flag", step.needsResolve && !globalFlagTest(flags, sizeof(flags), 7));
    conversationResolve(&conv, false, flags, sizeof(flags));
    check("... a failed service leaves the flag clear", !globalFlagTest(flags, sizeof(flags), 7));
    step = conversationChoose(&conv, 1, flags, sizeof(flags));
    conversationResolve(&conv, true, flags, sizeof(flags));
    check("... a successful one sets it", globalFlagTest(flags, sizeof(flags), 7));
    step = conversationChoose(&conv, 0, flags, sizeof(flags));
    check("Escape picks the last topic listed, which ends the conversation with its exit code", step.topic == 2 && step.ended && conv.ended && conv.exitCode == 2);
    check("nothing more can be chosen", conversationChoose(&conv, 1, flags, sizeof(flags)).topic == 0);
}

int main(void) {
    testBookkeeping();
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}

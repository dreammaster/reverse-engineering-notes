#ifndef YENDOR23_CONVERSATION_H
#define YENDOR23_CONVERSATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dialog.h"

/*
 * A talk with an NPC: the control loop of UseItem (yendor2.asm:13168; Chapter 3 the same) without a screen. LoadItemData (:22216) picks the opening topic
 * (dialogOpeningTopic), clears the two availability masks and the "type word", and sets bit 0 of mask B when the NPC's required item range is in the party;
 * UseItem then runs the chosen topic -- first the handler its type word names (healer, trainer, mounts, challenge: dialog.h's DialogService), otherwise the topic's
 * own flags (visit it: dialogVisitTopic; a DialogTopicPreview topic loads its argument as the type word) -- and its tail: attribute / experience tomes,
 * the key, enhance, repair and ore-buying services, and the end of the conversation. DrawEligibleItemList then lists up to ten topics of the NPC that
 * dialogTopicListed approves (two columns of five), numbered 1-9 for the keyboard; Escape picks the last one listed.
 *
 * conversationChoose does the part that is pure bookkeeping -- the topic's masks and flags, the type word, the end -- and reports in a ConversationStep what is left
 * for the caller to run: the service the topic or the type word selects (the screens of the original; the decision modules are dialogservice.h, shop.h and
 * the rest) and, for a DialogTopicConditional topic, a conversationResolve once the service says whether it succeeded (a right riddle answer, a completed purchase):
 * its result flags wait for that.
 *
 * The NPC's speaker mode (DialogNpcSpeakerMode, header word +2) nonzero means LoadItemData first asks which party member talks (a member who is not incapacitated);
 * conversationNeedsSpeaker tells, the caller keeps the member for the services.
 */
enum { ConversationListMax = 10 };

typedef struct {
    const DialogCatalog *catalog;
    unsigned npcId;
    const uint8_t *npc;
    DialogState masks;
    uint16_t typeWord;        /* word_2E410 */
    unsigned current;         /* the absolute index of the topic just run */
    int exitCode;             /* word_2E52A: 0 normally, the ending topic's argument when it is 1 or 2 */
    bool ended;
    bool pendingConditional;  /* the current topic's result flags wait for conversationResolve */
    unsigned listed[ConversationListMax]; /* absolute topic indices, in the order listed; listedCount of them */
    unsigned listedCount;
} Conversation;

/* What the topic that was just run asks of the caller. */
typedef struct {
    unsigned topic;           /* absolute index */
    uint16_t flags;           /* its DialogTopicFlags */
    uint16_t arg;             /* its DialogTopicArg */
    DialogService typeService; /* the handler the type word named while the topic ran (DialogServiceNone: a plain topic) */
    bool buy;                 /* flag 0x80 and the name starts "BUY ": an ore purchase; a 0x80 topic otherwise is a riddle */
    bool needsResolve;        /* DialogTopicConditional: call conversationResolve */
    bool ended;               /* the conversation is over (see Conversation.exitCode) */
} ConversationStep;

bool conversationNeedsSpeaker(const uint8_t *npc);

/*
 * LoadItemData + the first pass of UseItem's loop on the opening topic. requiredItemHeld: the party holds an item in DialogNpcRequiredItemRange (itemRangeAvailable).
 * False if the NPC does not exist.
 */
bool conversationStart(Conversation *conv, const DialogCatalog *catalog, unsigned npcId, uint8_t *globalFlags, size_t flagsSize, bool requiredItemHeld,
                       ConversationStep *first);

/* Recomputes `listed` from the masks and the global flags (conversationStart / Choose do it). */
void conversationList(Conversation *conv, const uint8_t *globalFlags, size_t flagsSize);

/* Runs the topic with this 1-based number in the list (0 = the last one listed, Escape's choice). A number past the list is ignored: step.topic is then 0. */
ConversationStep conversationChoose(Conversation *conv, unsigned number, uint8_t *globalFlags, size_t flagsSize);

/* The service of a conditional topic has finished: its result flags apply if it succeeded. */
void conversationResolve(Conversation *conv, bool success, uint8_t *globalFlags, size_t flagsSize);

/* The text the topic shows (dialogTopicText), at most capacity - 1 characters; '%' marks a line break and '~' is the font's apostrophe. */
size_t conversationText(const Conversation *conv, unsigned topic, char *out, size_t capacity);

#endif

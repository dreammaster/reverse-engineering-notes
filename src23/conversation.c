#include "conversation.h"

#include <string.h>

#include "globalflags.h"

bool conversationNeedsSpeaker(const uint8_t *npc) {
    return dialogGetU16(npc, DialogNpcSpeakerMode) != 0;
}

static bool flagOfNpc(uint16_t id, const uint8_t *globalFlags, size_t flagsSize) {
    return id != 0 && globalFlagTest(globalFlags, flagsSize, id);
}

/* The topic's result flags: ApplyItemEffectFlags' six signed ids. */
static void applyResultFlags(const DialogCatalog *catalog, const uint8_t *topic, uint8_t *globalFlags, size_t flagsSize) {
    for (unsigned i = 0; i < DialogTopicFlagSlots; i++) {
        globalFlagApplySigned(globalFlags, flagsSize, (int16_t)dialogTopicU16(catalog, topic, (DialogTopicField)(DialogTopicResultFirst + i * 2)));
    }
}

static ConversationStep run(Conversation *conv, unsigned topicIndex, uint8_t *globalFlags, size_t flagsSize) {
    ConversationStep step;
    memset(&step, 0, sizeof(step));
    const uint8_t *topic = dialogTopic(conv->catalog, topicIndex);
    if (!topic) {
        return step;
    }
    step.topic = topicIndex;
    step.flags = dialogGetU16(topic, DialogTopicFlags);
    step.arg = dialogTopicU16(conv->catalog, topic, DialogTopicArg);
    step.typeService = dialogServiceForType(conv->typeWord);
    if (step.flags & DialogTopicBuy) {
        char name[DialogTopicNameSize + 1];
        dialogTopicName(topic, name);
        step.buy = strncmp(name, "BUY ", 4) == 0;
    }
    /* masks and bookkeeping now; the result flags too, unless the topic is conditional on its service */
    step.needsResolve = (step.flags & DialogTopicConditional) != 0;
    dialogVisitTopic(conv->catalog, topic, &conv->masks, globalFlags, flagsSize, !step.needsResolve);
    conv->pendingConditional = step.needsResolve;
    conv->typeWord = dialogTypeAfterTopic(conv->typeWord, conv->catalog, topic);
    conv->current = topicIndex;
    if (step.flags & DialogTopicEndsConversation) {
        conv->ended = true;
        if (step.arg <= 2) {
            conv->exitCode = step.arg;
        }
    }
    step.ended = conv->ended;
    conversationList(conv, globalFlags, flagsSize);
    return step;
}

void conversationList(Conversation *conv, const uint8_t *globalFlags, size_t flagsSize) {
    conv->listedCount = 0;
    unsigned first = dialogGetU16(conv->npc, DialogNpcFirstTopic), count = dialogGetU16(conv->npc, DialogNpcTopicCount);
    for (unsigned n = 0; n < count && conv->listedCount < ConversationListMax; n++) {
        const uint8_t *topic = dialogTopic(conv->catalog, first + n);
        if (topic && dialogTopicListed(conv->catalog, topic, &conv->masks, globalFlags, flagsSize)) {
            conv->listed[conv->listedCount++] = first + n;
        }
    }
}

bool conversationStart(Conversation *conv, const DialogCatalog *catalog, unsigned npcId, uint8_t *globalFlags, size_t flagsSize, bool requiredItemHeld,
                       ConversationStep *first) {
    memset(conv, 0, sizeof(*conv));
    conv->catalog = catalog;
    conv->npcId = npcId;
    conv->npc = dialogNpc(catalog, npcId);
    if (!conv->npc) {
        return false;
    }
    if (requiredItemHeld && dialogGetU16(conv->npc, DialogNpcRequiredItemRange)) {
        conv->masks.availB |= 1;
    }
    bool a = flagOfNpc(dialogGetU16(conv->npc, DialogNpcGreetingFlagA), globalFlags, flagsSize);
    bool b = flagOfNpc(dialogGetU16(conv->npc, DialogNpcGreetingFlagB), globalFlags, flagsSize);
    bool c = flagOfNpc(dialogGetU16(conv->npc, DialogNpcGreetingFlagC), globalFlags, flagsSize);
    ConversationStep step = run(conv, dialogOpeningTopic(conv->npc, a, b, c), globalFlags, flagsSize);
    if (first) {
        *first = step;
    }
    return true;
}

ConversationStep conversationChoose(Conversation *conv, unsigned number, uint8_t *globalFlags, size_t flagsSize) {
    ConversationStep none;
    memset(&none, 0, sizeof(none));
    if (conv->ended || conv->listedCount == 0) {
        return none;
    }
    if (number == 0) {
        number = conv->listedCount;
    }
    if (number > conv->listedCount) {
        return none;
    }
    return run(conv, conv->listed[number - 1], globalFlags, flagsSize);
}

void conversationResolve(Conversation *conv, bool success, uint8_t *globalFlags, size_t flagsSize) {
    if (!conv->pendingConditional) {
        return;
    }
    conv->pendingConditional = false;
    if (success) {
        const uint8_t *topic = dialogTopic(conv->catalog, conv->current);
        if (topic) {
            applyResultFlags(conv->catalog, topic, globalFlags, flagsSize);
        }
    }
    conversationList(conv, globalFlags, flagsSize);
}

unsigned conversationRows(const Conversation *conv, unsigned topic, char rows[][DialogLineChars + 1], unsigned maxRows) {
    const uint8_t *t = dialogTopic(conv->catalog, topic);
    return t ? dialogTopicRows(conv->catalog, conv->npc, t, rows, maxRows) : 0;
}

size_t conversationText(const Conversation *conv, unsigned topic, char *out, size_t capacity) {
    const uint8_t *t = dialogTopic(conv->catalog, topic);
    return t ? dialogTopicText(conv->catalog, conv->npc, t, out, capacity) : 0;
}

#include "dialog.h"

#include <string.h>

static const DialogLayout g_layoutYendor2 = {0x1677A1,
                                             0x168F11,
                                             0x1771DB,
                                             DialogNpcCountYendor2,
                                             DialogTopicCountYendor2,
                                             DialogLineCountYendor2,
                                             DialogTopicRecordSizeYendor2};
static const DialogLayout g_layoutYendor3 = {0x3D8EB9,
                                             0x3DA4C1,
                                             0x3EA03D,
                                             DialogNpcCountYendor3,
                                             DialogTopicCountYendor3,
                                             DialogLineCountYendor3,
                                             DialogTopicRecordSizeYendor3};

const DialogLayout *dialogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool dialogCatalogParseWorldDat(DialogCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const DialogLayout *layout = dialogLayout(game);
    if (!layout) {
        return false;
    }
    size_t npcBytes = (size_t)layout->npcCount * DialogNpcRecordSize;
    size_t topicBytes = (size_t)layout->topicCount * layout->topicRecordSize;
    size_t lineBytes = (size_t)layout->lineCount * DialogLineSize;
    if (size < layout->npcOffset + npcBytes || size < layout->topicOffset + topicBytes ||
        size < layout->lineOffset + lineBytes) {
        return false;
    }
    memset(catalog, 0, sizeof(*catalog));
    catalog->game = game;
    catalog->npcCount = layout->npcCount;
    catalog->topicCount = layout->topicCount;
    catalog->lineCount = layout->lineCount;
    catalog->topicRecordSize = layout->topicRecordSize;
    memcpy(catalog->npcs, worldDat + layout->npcOffset, npcBytes);
    memcpy(catalog->topics, worldDat + layout->topicOffset, topicBytes);
    memcpy(catalog->lines, worldDat + layout->lineOffset, lineBytes);
    return true;
}

const uint8_t *dialogNpc(const DialogCatalog *catalog, unsigned id) {
    if (id == 0 || id >= catalog->npcCount) {
        return NULL;
    }
    return catalog->npcs + (size_t)id * DialogNpcRecordSize;
}

const uint8_t *dialogTopic(const DialogCatalog *catalog, unsigned index) {
    if (index >= catalog->topicCount) {
        return NULL;
    }
    return catalog->topics + (size_t)index * catalog->topicRecordSize;
}

const uint8_t *dialogLine(const DialogCatalog *catalog, unsigned index) {
    if (index >= catalog->lineCount) {
        return NULL;
    }
    return catalog->lines + (size_t)index * DialogLineSize;
}

uint16_t dialogGetU16(const uint8_t *record, unsigned offset) {
    return (uint16_t)(record[offset] | (record[offset + 1] << 8));
}

uint16_t dialogTopicU16(const DialogCatalog *catalog, const uint8_t *topic, DialogTopicField field) {
    unsigned offset = (unsigned)field;
    if (catalog->game == GameYendor3 && offset >= DialogTopicArg) {
        offset += 2;
    }
    return dialogGetU16(topic, offset);
}

void dialogTopicName(const uint8_t *topic, char *out) {
    unsigned len = DialogTopicNameSize;
    while (len > 0 && (topic[len - 1] == 0 || topic[len - 1] == ' ')) {
        len--;
    }
    memcpy(out, topic, len);
    out[len] = '\0';
}

size_t dialogTopicText(const DialogCatalog *catalog, const uint8_t *npc, const uint8_t *topic, char *out, size_t capacity) {
    unsigned lines = dialogTopicU16(catalog, topic, DialogTopicTextLines);
    unsigned first = dialogGetU16(npc, DialogNpcFirstLine) + dialogTopicU16(catalog, topic, DialogTopicTextOffset) / DialogLineSize;
    size_t length = 0;
    size_t trimmed = 0; /* length with trailing spaces dropped */
    for (unsigned i = 0; i < lines; i++) {
        const uint8_t *line = dialogLine(catalog, first + i);
        if (!line) {
            break;
        }
        for (unsigned c = 0; c < DialogLineChars && line[c] != 0; c++) {
            if (capacity > 0 && length + 1 < capacity) {
                out[length] = (char)line[c];
            }
            length++;
            if (line[c] != ' ') {
                trimmed = length;
            }
        }
    }
    if (capacity > 0) {
        out[trimmed < capacity - 1 ? trimmed : capacity - 1] = '\0';
    }
    return trimmed;
}

unsigned dialogOpeningTopic(const uint8_t *npc, bool flagA, bool flagB, bool flagC) {
    unsigned first = dialogGetU16(npc, DialogNpcFirstTopic);
    if (flagA) {
        return first + 1;
    }
    if (flagB) {
        return first + 2;
    }
    if (flagC) {
        return first + 3;
    }
    return first;
}

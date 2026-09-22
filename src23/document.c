#include "document.h"

#include <string.h>

const uint8_t DocumentLineWidth[DocumentCategoryCount] = {16, 23, 22, 23};

static const DocumentCatalogLayout g_layoutYendor2 = {0x15181C, DocumentRegionSizeYendor2};
static const DocumentCatalogLayout g_layoutYendor3 = {0x3C2030, DocumentRegionSizeYendor3};

/*
 * {region-relative byte offset, line count}, extracted from the
 * LookupConversationTextBlockOffset_* stubs' own index tables (baked into
 * the executable; yendor2.asm:42441 on, yendor3.asm:42823 on) and
 * converted from absolute WORLD.DAT offsets to offsets relative to each
 * game's own region base above. A 0-line entry (Note/Unused in Chapter 2,
 * most of Document in Chapter 3) is real: the game's own index table has
 * length 0 there too, not a parsing gap.
 */
typedef struct {
    uint16_t offset;
    uint16_t lines;
} DocumentEntry;

static const DocumentEntry g_journalYendor2[DocumentCountYendor2Journal] = {
    {0, 78}, {1248, 78}, {2496, 104}, {4160, 52}, {4992, 104}, {6656, 26},
};
static const DocumentEntry g_noteYendor2[DocumentCountYendor2Note] = {
    {7072, 0},
};
static const DocumentEntry g_documentYendor2[DocumentCountYendor2Document] = {
    {7072, 11}, {7314, 33},  {8040, 11},  {8282, 11},  {8524, 11},  {8766, 11},  {9008, 11},  {9250, 11},
    {9492, 11}, {9734, 11},  {9976, 11},  {10218, 11}, {10460, 11}, {10702, 11}, {10944, 11}, {11186, 22},
    {11670, 22}, {12154, 11}, {12396, 11}, {12638, 11}, {12880, 11}, {13122, 11}, {13364, 11}, {13606, 11},
    {13848, 11}, {14090, 11},
};
static const DocumentEntry g_unusedYendor2[DocumentCountYendor2Unused] = {
    {DocumentRegionSizeYendor2, 0}, /* points one past the region's end, like every other empty entry */
};

static const DocumentEntry g_journalYendor3[DocumentCountYendor3Journal] = {
    {0, 52}, {832, 78}, {2080, 91}, {3536, 52}, {4368, 39}, {4992, 52}, {5824, 39}, {6448, 26},
};
static const DocumentEntry g_noteYendor3[DocumentCountYendor3Note] = {
    {6864, 16}, {7232, 16}, {7600, 16}, {7968, 16}, {8336, 16}, {8704, 16}, {9072, 16}, {9440, 16},
};
static const DocumentEntry g_documentYendor3[DocumentCountYendor3Document] = {
    {9808, 33}, {10534, 22}, {11018, 11}, {11260, 11}, {11502, 11}, {11744, 11}, {11986, 0}, {11986, 0},
    {11986, 0}, {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},
    {11986, 0}, {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},  {11986, 0},
    {11986, 0}, {11986, 0},
};
static const DocumentEntry g_unusedYendor3[DocumentCountYendor3Unused] = {
    {11986, 0},
};

typedef struct {
    const DocumentEntry *entries;
    unsigned count;
} DocumentTable;

static DocumentTable tableFor(GameKind game, DocumentCategory category) {
    if (game == GameYendor2) {
        switch (category) {
        case DocumentCategoryJournal:
            return (DocumentTable){g_journalYendor2, DocumentCountYendor2Journal};
        case DocumentCategoryNote:
            return (DocumentTable){g_noteYendor2, DocumentCountYendor2Note};
        case DocumentCategoryDocument:
            return (DocumentTable){g_documentYendor2, DocumentCountYendor2Document};
        case DocumentCategoryUnused:
            return (DocumentTable){g_unusedYendor2, DocumentCountYendor2Unused};
        case DocumentCategoryCount:
            break;
        }
    } else if (game == GameYendor3) {
        switch (category) {
        case DocumentCategoryJournal:
            return (DocumentTable){g_journalYendor3, DocumentCountYendor3Journal};
        case DocumentCategoryNote:
            return (DocumentTable){g_noteYendor3, DocumentCountYendor3Note};
        case DocumentCategoryDocument:
            return (DocumentTable){g_documentYendor3, DocumentCountYendor3Document};
        case DocumentCategoryUnused:
            return (DocumentTable){g_unusedYendor3, DocumentCountYendor3Unused};
        case DocumentCategoryCount:
            break;
        }
    }
    return (DocumentTable){NULL, 0};
}

const DocumentCatalogLayout *documentCatalogLayout(GameKind game) {
    switch (game) {
    case GameYendor2:
        return &g_layoutYendor2;
    case GameYendor3:
        return &g_layoutYendor3;
    }
    return NULL;
}

bool documentCatalogParse(DocumentCatalog *catalog, GameKind game, const uint8_t *region, size_t size) {
    const DocumentCatalogLayout *layout = documentCatalogLayout(game);
    if (!layout || size < layout->regionSize) {
        return false;
    }
    memset(catalog, 0, sizeof(*catalog));
    catalog->game = game;
    memcpy(catalog->region, region, layout->regionSize);
    return true;
}

bool documentCatalogParseWorldDat(DocumentCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size) {
    const DocumentCatalogLayout *layout = documentCatalogLayout(game);
    if (!layout) {
        return false;
    }
    size_t needed = (size_t)layout->regionOffset + layout->regionSize;
    if (size < needed) {
        return false;
    }
    return documentCatalogParse(catalog, game, worldDat + layout->regionOffset, layout->regionSize);
}

unsigned documentCount(GameKind game, DocumentCategory category) {
    return tableFor(game, category).count;
}

unsigned documentLineCount(GameKind game, DocumentCategory category, unsigned id) {
    DocumentTable table = tableFor(game, category);
    if (id == 0 || id > table.count) {
        return 0;
    }
    return table.entries[id - 1].lines;
}

bool documentGetLine(const DocumentCatalog *catalog, DocumentCategory category, unsigned id, unsigned lineIndex,
                      char out[DocumentLineBufferSize]) {
    DocumentTable table = tableFor(catalog->game, category);
    if (id == 0 || id > table.count) {
        return false;
    }
    const DocumentEntry *entry = &table.entries[id - 1];
    if (lineIndex >= entry->lines) {
        return false;
    }

    uint8_t width = DocumentLineWidth[category];
    const uint8_t *line = catalog->region + entry->offset + (size_t)lineIndex * width;
    size_t length = 0;
    while (length < (size_t)width - 1 && line[length] != 0) {
        length++;
    }
    memcpy(out, line, length);
    out[length] = '\0';
    return true;
}

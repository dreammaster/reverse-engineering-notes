#ifndef YENDOR23_DOCUMENT_H
#define YENDOR23_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * In-world readable text: journal/history entries and verses, short notes,
 * and longer found documents -- everything RunConversation displays
 * (yendor2.asm:49793). Despite the name (an old, unverified guess), this
 * is not NPC dialogue: RunConversation is reached from HandleGameCommand's
 * item-use dispatch, driven by the same word_2E548 item-classification
 * record RepairItemCommand/InteractWithContainer use right next to it in
 * the same function -- using/reading an in-world item (a book, sign,
 * plaque, or note) is what triggers it. Confirmed by content, not just
 * structure: real text was read out of both games' real WORLD.DAT files
 * (see below).
 *
 * Four independent id-indexed pools ("categories" here; the game
 * distinguishes them only by which word_2E548 flag bit is set --
 * RunConversation+_4000/_2000/_1000/_800 in the disassembly). Each pool is
 * a small (id -> WORLD.DAT byte offset, line count) index table baked into
 * the executable itself (not WORLD.DAT) -- LookupConversationTextBlockOffset_*,
 * yendor2.asm:42441 on. The id itself comes from the triggering item's own
 * catalog data (not yet traced which field). Both games' four pools'
 * *text* turned out to live in one small contiguous WORLD.DAT span each
 * (Ch2: 14,332 bytes at 0x15181C; Ch3: 11,986 bytes at 0x3C2030) even
 * though each pool has its own separate index table -- confirmed by every
 * pool's entries exactly tiling that span with no gaps.
 *
 * Record format: fixed-width lines, `DocumentLineWidth[category] - 1`
 * characters then a NUL (never wrapped mid-word -- pre-wrapped at
 * authoring time, matching the pre-wrapped-line convention already noted
 * for DrawWordToken). Line count = byte length / line width, exact with a
 * zero remainder for every real entry in both games -- a strong structural
 * check that this is the right width per category. Spacing (including
 * leading/trailing spaces used to center short lines, e.g. Chapter 3's
 * "   %THE BLACK CAT%   ") is meaningful and preserved by documentGetLine,
 * not trimmed.
 *
 * Content observed directly (not just inferred from structure): Journal
 * mixes dated "MM/DD/YYY" history entries and undated verses/poems ("THE
 * FOREST IS WHERE I LONG TO BE..."); Note holds short messages, including
 * at least one that is literally a spoken password ("THE PASSWORD IS
 * `RUSE~" -- worth revisiting against the "region password" open
 * question in roadmap.md); Document holds longer found documents, letters
 * and short stories (one Chapter 3 entry is signed "BY JASPER"). These
 * category names describe what was actually found, not a confirmed
 * official taxonomy. Unused has no real entries in either game examined --
 * wired up in the engine but empty content, like a few other
 * engine-supports-more-than-this-chapter-uses cases already seen
 * elsewhere (see engine-diffs.md).
 */

typedef enum {
    DocumentCategoryJournal,  /* was RunConversation's "_4000" branch; 16-byte lines */
    DocumentCategoryNote,     /* "_1000"; 23-byte lines; empty in Chapter 2's real data */
    DocumentCategoryDocument, /* "_2000"; 22-byte lines */
    DocumentCategoryUnused,   /* "_800"; 23-byte lines; empty in both games examined */
    DocumentCategoryCount
} DocumentCategory;

enum {
    DocumentLineBufferSize = 23, /* the largest DocumentLineWidth entry */

    DocumentCountYendor2Journal = 6,
    DocumentCountYendor2Note = 1,
    DocumentCountYendor2Document = 26,
    DocumentCountYendor2Unused = 1,

    DocumentCountYendor3Journal = 8,
    DocumentCountYendor3Note = 8,
    DocumentCountYendor3Document = 26,
    DocumentCountYendor3Unused = 1,

    DocumentCountMax = 26,

    DocumentRegionSizeYendor2 = 14332,
    DocumentRegionSizeYendor3 = 11986,
    DocumentRegionSizeMax = DocumentRegionSizeYendor2
};

/* Bytes per line, NUL included (DocumentLineWidth[c] - 1 characters of real content). */
extern const uint8_t DocumentLineWidth[DocumentCategoryCount];

typedef struct {
    uint32_t regionOffset; /* WORLD.DAT byte offset of the whole text span */
    uint32_t regionSize;
} DocumentCatalogLayout;

typedef struct {
    GameKind game;
    uint8_t region[DocumentRegionSizeMax];
} DocumentCatalog;

const DocumentCatalogLayout *documentCatalogLayout(GameKind game);

/* Parses regionSize bytes starting at region[0]; false if size is too small. */
bool documentCatalogParse(DocumentCatalog *catalog, GameKind game, const uint8_t *region, size_t size);

/* Same, from a whole WORLD.DAT image already in memory. */
bool documentCatalogParseWorldDat(DocumentCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

/* How many ids a category has (1-based; some may be empty -- see documentLineCount). */
unsigned documentCount(GameKind game, DocumentCategory category);

/* Lines in entry id; 0 if id is out of range or the entry is empty. */
unsigned documentLineCount(GameKind game, DocumentCategory category, unsigned id);

/*
 * Copies one line's text (up to DocumentLineWidth[category]-1 characters,
 * NUL-terminated, spacing preserved) into out. False if the category/id/line
 * is out of range.
 */
bool documentGetLine(const DocumentCatalog *catalog, DocumentCategory category, unsigned id, unsigned lineIndex,
                      char out[DocumentLineBufferSize]);

#endif

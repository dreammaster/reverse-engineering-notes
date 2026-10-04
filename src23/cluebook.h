#ifndef YENDOR23_CLUEBOOK_H
#define YENDOR23_CLUEBOOK_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * The on-line clue book's navigation bar: DrawClueBookNavBar (yendor2.asm:7728, yendor3.asm:15479). `flags` is g_clueBookNavFlags:
 * bit 0x40 shows the hint "d LIST" at (11, 185), bit 0x20 the hint "MAP c" at (272, 185) (font 0, colour 0xF, transparent), and exactly
 * one of 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200 marks the current category tab. The seven tabs are 16 x 16 icons
 * (category 8) at x = 62 + 30k, y = 180; a tab's picture is its base id, +1 when it is the selected one. Base ids: Chapter 2
 * 0x20, 0x145, 0x147, 0x153, 0x149, 0x14B, 0x14D; Chapter 3 0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2A.
 */
enum { ClueTabCount = 7, ClueNavHintList = 0x40, ClueNavHintMap = 0x20, ClueTabFirstBit = 0x8000 };

void clueNavBarDraw(const ViewRenderer *r, uint16_t flags);

/* The picture id of tab `tab` (0-6) for the flags. */
unsigned clueTabPicture(GameKind game, unsigned tab, uint16_t flags);

/*
 * DrawClueEntryList (yendor2.asm:4543): up to 14 entry names (the ClueEntries region table: x = 40, y = 27 + 10 row) in font 0, transparent:
 * the selected row 0x8A, the others 0x0A; an entry the party has not identified (`known` false) while `revealAll` is off is drawn in 0x84
 * (selected) or 0x05 (others). The names themselves come from BuildClueEntryText (not reimplemented here).
 */
enum { ClueListRows = 14 };

typedef struct {
    const char *name; /* NULL = blank row */
    bool known;
} ClueListRow;

void clueEntryListDraw(const ViewRenderer *r, const ClueListRow *rows, unsigned count, int selected, bool revealAll);

/*
 * BuildClueEntryText (yendor2.asm:4808, yendor3 identical): where a category's entry names come from. Categories (g_clueBookCategory) 1-0x11:
 * 1 map locations (BuildClueLocationSuffix), 2 monsters (BuildMonsterDisplayName), 3 and 5-10 spells/abilities (LoadClueBookSpellEntry), 4 and 11
 * the n-th string of two packed string tables in the executable, 12-17 items (BuildItemDisplayName); anything else has no text.
 */
typedef enum { ClueSourceNone, ClueSourceLocation, ClueSourceMonster, ClueSourceSpell, ClueSourcePackedA, ClueSourcePackedB, ClueSourceItem } ClueTextSource;

ClueTextSource clueCategorySource(unsigned category);

/*
 * The category heading in the top right corner of every clue page (the second string of DrawMessageBox, colour 0xD, y = 4). Chapter 3 right
 * aligns it to x = 313 (x = 313 - 6 * length); Chapter 2 passes an explicit x per category that falls within 4 pixels of the same rule
 * (MAPS 291, MONSTER STATISTICS 208, SPELL INFORMATION 213, MAGIC USER INFORMATION 183, INVENTORY ITEMS 225, ARMOR/RINGS 249,
 * JEWELS/ARTIFACTS/UNIQUE ITEMS 141, MAGIC SCROLLS/QUARTZ 195, POTIONS 273, SUPPLIES/FOOD 237, WEAPONS 273). Categories as
 * clueCategorySource: 1 maps, 2 monsters, 3 spells, 4 magic users, 11 items list, 12-17 item pages.
 */
int clueHeadingX(GameKind game, unsigned category, unsigned length);

/*
 * The entry list's paging state (ShowClueCategoryEntries yendor2.asm:4758, HandleClueEntryScrollInput :4690, HandleClueEntryRowScrollInput :4617,
 * ScrollClueEntryListPageUp/Down, RecomputeClueEntryPageBounds; the original keeps byte pointers into a list of 4-byte entries, here they are entry
 * indices). A page shows ClueListRows (14) entries from `first` to `last`. The up / down hints of the navigation bar (bits 0x100 / 0x80) are set when
 * the list is longer than a page and `first` is not the first entry / `last` is not the final one. The list page's backdrop is category 0
 * picture 14 (the item and monster pages use 13).
 *
 * Keys: I and Q page up / down (the selection keeps its row in the page; with no further page they jump to the first / last row of this one),
 * H and P move the selection one row, turning the page at the edge (the new page's last / first row is selected). The return value is the original's
 * errorCode: 0 nothing, 1 the selection moved up, 2 down.
 */
typedef struct {
    unsigned count;
    unsigned first, last, selected;
} ClueList;

void clueListInit(ClueList *list, unsigned count);
bool clueListCanPageUp(const ClueList *list);
bool clueListCanPageDown(const ClueList *list);
unsigned clueListPageKey(ClueList *list, bool down);
unsigned clueListRowKey(ClueList *list, bool down);

#endif

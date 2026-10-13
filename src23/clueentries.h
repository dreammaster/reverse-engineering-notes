#ifndef YENDOR23_CLUEENTRIES_H
#define YENDOR23_CLUEENTRIES_H

#include <stdbool.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"

/*
 * The clue book's entry lists (ShowClueCategoryEntries yendor2.asm:4757, DrawClueEntryList :4542, BuildClueEntryText :4807): the executable's data segment holds, for each of the
 * 17 categories, a list -- a table of 17 near pointers at DS:0xF3F4 (Chapter 3 0xF6A8), each pointing at a word count followed by that many 4-byte entries. An entry is the id
 * the category's names and pages are looked up by (word 0) and a flag word (word 1) whose bit 0x8000 marks the entries the shareware program lets the player open: an entry without it
 * is drawn dim (colours 0x84 selected / 0x05) and RunClueEntryMenu answers a choice of it with the registration reminder, unless the program is registered (flag bit 0 of
 * g_uiScratchFlags4), where every entry is bright and opens. The ids by category:
 *   1 map locations (the map id, block n - 1: location.h)   2 monsters (the monster type id)   3 spells (spell id)   4 the six magic user classes (1-6: the first packed string table)
 *   5-10 the spells of class 1-6 (spell id; chosen from category 4: category 4 + class)   11 the eight item groups (1-8: the second packed string table)
 *   12 armor and rings, 13 jewels and artifacts, 14 scrolls and quartz, 15 potions, 16 supplies and food, 17 weapons (item catalog ids; item group 2, the attribute enhancers, has
 *   no list -- its screen is the legend of clueitem.h -- and group 7 is the transportation page).
 * The two packed string tables (NUL-separated, read in order) hold the class names ("MONK / CLERIC / PRIEST", ...) and the item group names; BuildClueEntryText walks them with
 * AdvanceToNextPackedString.
 */
enum { ClueEntriesMax = 128, ClueEntryTextSize = 48 };

typedef struct {
    uint16_t id;
    bool free; /* the shareware program opens it */
} ClueEntry;

/* The entries of `category` (1-17); returns the count (0 for another category or a damaged table). */
unsigned clueEntries(const ExeData *exe, GameKind game, unsigned category, ClueEntry out[ClueEntriesMax]);

/* The n-th (1-based) string of the class table (category 4) or the item group table (category 11). */
bool clueEntryPackedString(const ExeData *exe, GameKind game, unsigned category, unsigned index, char out[ClueEntryTextSize]);

#endif

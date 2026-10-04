#ifndef YENDOR23_CLUEITEM_H
#define YENDOR23_CLUEITEM_H

#include <stdbool.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"
#include "item.h"
#include "viewrender.h"

/*
 * The clue book's item pages (F5 INVENTORY ITEMS; ShowClueBookItemDetail yendor2.asm:5668, the armor / weapon / healing / duration rows
 * after it and their helpers; Chapter 3 the same apart from the healing row). The labels are read from the executable's data segment
 * (exedata.h) -- they are the game's text, so nothing here carries them -- from the addresses below.
 *
 * Page layout (font 0, transparent text; label colour 0x0A, values as listed):
 *   backdrop   screen cleared to colour 0, PICTURES category 0 picture 13 (Chapter 3: 6) at (1, 1); the item's display name (itemGetName) at (6, 4) in 0xD and the category heading at the top right (cluebook.h);
 *              the clue navigation bar (cluebook.h)
 *   icon       category 8 picture [item +8] (+1 with ItemFlagAltIcon) at (68, 41)
 *   (91, 39)   BASE VALUE: label; the base value (comma grouped, bcd4Format) at x = 157 in 0x8A when not zero
 *   (115, 45)  WEIGHT: label; the weight with one decimal ("12.5": the last digit after a '.') at x = 157 in 0x8A when not zero
 *   (110, 69)  FITS IN- ; at x = 158: the container list in 0xCA -- with no fit bits CHARACTER PANEL (ItemFlagEquipCode0B) or ANY PANEL;
 *              with fit bits BACKPACK (0x8000, then +54), BOX (0x4000, then +24) and BAG (0x2000) one after another
 * Rows by kind (entry = the item's target entry, itemTargetEntry; value colour 0x59, text 0xA7, numbers drawn at x = 157):
 *   armor      (91, 57) ABSORPTION- entry word 0; PROTECTIONS: at (85, 81) and ADDS: at (127, 111) followed by the item's effect pairs, one per
 *              line 6 pixels apart from the label's own line: amount at x = 157, the name at x = 181. A pair whose field offset is
 *              0x20-0x30 is a protection (name = table entry (offset - 0x20) / 2); 0x7C and up an attribute / skill bonus (name = table entry
 *              (offset - 0x7C) / 2); the list ends at the first zero offset (at most 4 pairs)
 *   weapon     (115, 57) DAMAGE: entry word 0; (121, 90) SKILL: the type picked by entry word 1 bit 0x8000 / 0x4000 / 0x2000 / 0x1000
 *              (PROJECTILE, SLASHING, BASHING, POLEARM; none = PROJECTILE); (103, 120) 2-HANDED: YES when word 1 bit 0 is set, else NO
 *   healing    (Chapter 2) HEALTH- at (115, 57), or MAGIC- at (121, 57) when word 1 has 0x8000, then entry word 2 (the percentage) and PERCENT
 *              at x = 175 (169 below 10) in 0xD; (Chapter 3) only the MAGIC- variant is shown
 *   duration   DURATION- at (103, 57), 10 x entry word 2 and MINUTES at x = 181 in 0xD
 * (The row of sub-icon selectors under armor / weapon pages is positioned by a table the game fills in at run time; not drawn here.)
 */
typedef struct {
    char baseValue[16], weight[12], absorption[16], fitsIn[12], adds[8], characterPanel[20], anyPanel[16], backpack[12], box[8], bag[8];
    char twoHanded[12], yes[6], no[6], skill[10], duration[12], minutes[10], health[10], magic[10], percent[10], damage[10], protections[16];
    char protectionNames[9][16];
    char statNames[27][16];
    char skillTypes[5][16];
    char headings[18][32]; /* by clue category (cluebook.h clueHeadingX): 11 INVENTORY ITEMS, 12-17 the item pages; others empty */
} ClueItemText;

/* Reads every label from the executable; false if one lies outside the file. */
bool clueItemTextLoad(ClueItemText *text, const ExeData *exe, GameKind game);

/* "1234" with one decimal -> "123.4", with 0 -> "1234" (the original's FormatNumber, StripCommasAndSpaces, InsertDecimalPointFromEnd). */
void clueFormatNumber(unsigned value, unsigned decimals, char out[16]);

/* The common part of the page: backdrop, name, navigation bar, icon, value, weight, fits-in row. */
void clueItemPageDraw(const ViewRenderer *r, const ClueItemText *text, const uint8_t *itemRecord, uint16_t navFlags, unsigned category);

void clueArmorRowDraw(const ViewRenderer *r, const ClueItemText *text, const uint8_t *wearableEntry, const uint8_t *effectEntry);
void clueWeaponRowDraw(const ViewRenderer *r, const ClueItemText *text, const uint8_t *weaponEntry);
void clueHealingRowDraw(const ViewRenderer *r, const ClueItemText *text, GameKind game, const uint8_t *consumableEntry);
void clueDurationRowDraw(const ViewRenderer *r, const ClueItemText *text, const uint8_t *consumableEntry);

#endif

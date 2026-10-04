#ifndef YENDOR23_CHARCREATE_H
#define YENDOR23_CHARCREATE_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "item.h"
#include "textfield.h"
#include "viewrender.h"

/*
 * The portrait step of character creation (ShowCharacterEquipment, yendor2.asm:36582 -- misnamed; it is the "pick a portrait" screen), the
 * same in both games. Nine portraits per gender (gender 1 = male, 2 = female). Choosing portrait k (1-9) stores, in the party record,
 * the paper-doll body picture [+0x14] = 2 * (k - 1) (+1 for a female) and the face picture [+0x12] = [+0x14] + 0x13 (PICTURES.VGA
 * category 6 body, category 7 face). The screen draws the nine faces of the current gender as a 3 x 3 grid of category 7 pictures, 32 x 32
 * at x = 8 + 33 column, y = 42 + 33 row, numbered row by row 1-9 (keys 1-9 or a click choose one; M / F switch gender).
 */
enum { CharCreatePortraits = 9, CharCreateFaceBase = 0x13 };

unsigned charCreateBodyPicture(unsigned gender, unsigned portrait);
unsigned charCreateFacePicture(unsigned gender, unsigned portrait);

/* Writes [+0x12] (face) and [+0x14] (body) of a party record for the choice. */
void charCreateChoosePortrait(uint8_t *partyRecord, unsigned gender, unsigned portrait);

/* The 3 x 3 grid of faces for `gender`. */
void charCreatePortraitGridDraw(const ViewRenderer *r, unsigned gender);

/*
 * The class step (ShowCharacterSkills, yendor2.asm:35989 -- misnamed): "PICK A CLASS" at (8, 25) in colour 0x8A and the nine classes in three
 * groups, each with a header in 0x8A at x = 8: "NON-MAGIC USERS:" (y 42) FIGHTER / MERCHANT / ROGUE, "CLERIC TYPES:" (y 87) MONK / ALCHEMIST /
 * PALADIN, "WIZARD TYPES:" (y 132) MAGE / DRUID / MARKSMAN, the names 9 pixels apart from y = 51 / 96 / 141 (x = 8). The hotkey letter of
 * each (F M R O A P G D K) is drawn in colour 0x7B, the rest in 0xF. Class ids 1-9 (party.h partyClassName).
 */
void charCreateClassPickDraw(const ViewRenderer *r);

/*
 * The bottom-left exit label (DrawQuitOrReturnLabel, yendor2.asm:37632): QUIT "CREATE" with the Q highlighted while creating a hero, or RETURN
 * (the E highlighted) when the same screens show an existing character. Drawn at (8, 185), the highlighted letter in 0x7B, the rest 0xF.
 * The class and item screens both end with it.
 */
void charCreateExitLabelDraw(const ViewRenderer *r, bool returning);

/*
 * The item pick (ShowCharacterInventory, yendor2.asm:36150; "TAKE UP TO FOUR" / "ITEMS" in 0x8A at (8, 25) and (8, 31)): up to eight items
 * in rows 16 pixels apart from y = 42, each the item's category 8 icon at (8, y) and its label at (25, y + 5) in colour 0xF. An item id of 0 or a
 * set bit in `hiddenMask` (0x80 for the first row down to 0x01 for the eighth: items already taken) leaves a row empty. Unless
 * `returning` (viewing an existing character) "NAME CHARACTER" is written at (8, 176) with its N highlighted, then the exit label.
 */
void charCreateItemLabel(const uint8_t *itemRecord, char out[2 * ItemNameLineSize + 2]);
void charCreateItemListDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint16_t itemIds[8], unsigned hiddenMask, bool returning);

/*
 * The roll screen (ShowCharacterStats, yendor2.asm:36743): the new hero's character sheet (statsheet.h characterSheetDraw) with "SELECT AN" and
 * "OPTION" in 0x8A at (8, 25) and (8, 31), "ROLL ATTRIBUTES" (R highlighted) at (8, 51), "PICK ITEMS" (the I, the sixth character, highlighted)
 * at (8, 69) and the creation exit label. R rolls again, I (or a click on the second line) accepts the roll, Q quits creation. Draw the
 * sheet first; this adds the option texts.
 */
void charCreateRollOptionsDraw(const ViewRenderer *r);

/*
 * The summary menu (ShowCharacterSummary, yendor2.asm:37176; the screen after the pick steps, over the character sheet whose title is
 * "CHARACTER CREATION", see characterSheetDraw): "SELECT AN" / "OPTION" in 0x8A at (8, 25) and (8, 31), then the six options with their
 * hotkey highlighted: KEEP CHARACTER (K) at y = 51, CLASS (C) 69, PORTRAIT (P) 78, ROLL ATTRIBUTES (R) 87, PICK ITEMS (I, the sixth
 * character) 96, NAME CHARACTER (N) 105, all at x = 8, and the exit label. K saves the hero (ApplySecondaryClassTierFlags, chargen.h),
 * Q discards it.
 */
void charCreateSummaryDraw(const ViewRenderer *r);

/*
 * The name prompt (EditCharacterName, yendor2.asm:36518): "ENTER THE NAME" in 0x8A at (8, 25) and the text field (textfield.h, 13 including
 * the terminator) at (8, 51) in colour 0xF on 0x33, opaque, drawn as the text, the '-' cursor and spaces to the field's full width.
 * A result that is empty after trimming trailing spaces is asked for again; otherwise it is stored in the record's name ([+0x00]) and also
 * written at (156, 26) in 0xF on the sheet header (done by the sheet drawing). Escape returns to the summary menu.
 */
enum { CharCreateNameFieldSize = 13 };
void charCreateNamePromptDraw(const ViewRenderer *r, const TextField *field);

/* Stores a confirmed name: trailing spaces trimmed, false (nothing stored) if nothing is left. */
bool charCreateAcceptName(uint8_t *partyRecord, const TextField *field);

#endif

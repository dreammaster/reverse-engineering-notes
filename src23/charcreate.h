#ifndef YENDOR23_CHARCREATE_H
#define YENDOR23_CHARCREATE_H

#include <stdint.h>

#include "game.h"
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

#endif

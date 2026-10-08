#ifndef YENDOR23_CREATION_H
#define YENDOR23_CREATION_H

#include <stdbool.h>
#include <stdint.h>

#include "chargen.h"
#include "charcreate.h"
#include "item.h"
#include "random.h"
#include "textfield.h"

/*
 * Making a hero without the screens (charcreate.c draws them; this is what the wizard's choices amount to): the portrait (charCreateChoosePortrait), the gender word
 * (+0x10: 1 male, 2 female), the class (partyBeginClassSelection + partyChooseClass: the attributes rolled, the skills derived, the ratings computed -- R on the roll
 * screen is creationReroll), the starting items and the name; keeping the character then runs ApplySecondaryClassTierFlags (partyApplyStartingAbilities).
 *
 * The item pick ("TAKE UP TO FOUR ITEMS", ShowCharacterInventory yendor2.asm:36150) always offers the same eight catalog items, constants InitGlobals loads
 * (Chapter 2 `_val1`-`_val8`: 0x21E 0x19C 0x1A2 0x1A8 0x16A 0x138 6 0x36; Chapter 3 DS:0xF18-0xF26: 8 0xE 0x12 0x16 4 0x19 0x1C 0x1F); up to four of them are moved into
 * the hero's inventory slots.
 */
enum { CreationItemChoices = 8, CreationItemsMax = 4 };

void creationItemIds(GameKind game, uint16_t out[CreationItemChoices]);

typedef struct {
    unsigned gender;    /* 1 male, 2 female */
    unsigned portrait;  /* 1-9 */
    unsigned classBase; /* 1-9 (party.h partyClassName) */
    unsigned items[CreationItemsMax]; /* indices 0-7 into creationItemIds; 8 = none (a repeat is taken once) */
    const char *name;   /* up to 12 characters; an empty name is refused */
} CreationChoice;

/* Builds the hero into a blank 500-byte record. False (the record is cleared) for a bad gender, portrait, class or an empty name. */
bool creationMakeHero(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng, const CreationChoice *choice);

/* The roll screen's R: new attributes for the chosen class (keeps the class, portrait, items and name). */
void creationReroll(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng);

#endif

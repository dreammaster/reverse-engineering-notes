#ifndef YENDOR23_ALCHEMYLIST_H
#define YENDOR23_ALCHEMYLIST_H

#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * The spell list of the alchemy/casting screen: DrawAlchemyConfirmDialogBackground and DrawAlchemySpellList (yendor2.asm:25580 / :25391,
 * Chapter 3: see below). The frame is category 1 picture 4 (transparent) at (15, 23); up to 13 rows follow from y = 37, 6 pixels apart:
 * the spell name at x = 21 in the row's colour, the MP cost at x = 150 and -- only when nonzero -- the NUORE cost at x = 179 and the
 * MAGIC ORE cost at x = 202 (plain decimal). The selected row is drawn in 0x8A when its colour is 0xF (castable), else 0x85.
 * Chapter 3 differs: frame picture 3, the MP cost at x = 170 and one further cost (the `nuore` field) at x = 203, no third column. The colour per
 * row is BuildAlchemySpellList's castability tint (party.h).
 */
enum { AlchemyListRows = 13 };

typedef struct {
    const char *name;
    uint8_t colour;
    unsigned mp, nuore, ore;
} AlchemyRow;

/* Draws the frame and the first AlchemyListRows rows; `selected` is a row index or -1. */
void alchemyListDraw(const ViewRenderer *r, const AlchemyRow *rows, unsigned count, int selected);

#endif

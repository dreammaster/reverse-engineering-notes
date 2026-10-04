#ifndef YENDOR23_ALCHEMYLIST_H
#define YENDOR23_ALCHEMYLIST_H

#include <stdint.h>

#include "bcd4.h"
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

/*
 * The status panel beside the alchemy screen (DrawAlchemyStatusPanel, yendor2.asm:25596; Chapter 3 :25583 differs only in showing one ore
 * counter): the text panel is cleared, then at x = 241 / 240 the character's name over a blank filler (colour 0x8A on 4) at y = 87; the
 * MAGIC: label at y = 96 in 0xCA (0xCC when the current MP is above the maximum); "current/max" in 0xF at y = 102; the first ore label
 * (MAGIC ORE:) in 0x8A at y = 114 with its counter (bcd4Format, 0xF) at y = 120, the second (NUORE:) at y = 132 with its counter at y = 138.
 * Chapter 3 passes only the second label and counter. The labels come from the executable (exedata.h: Chapter 2 DS:0x7955, 0x7B15, 0x7C61,
 * 0x7C6D; Chapter 3 0x7C87, 0x7E47, 0x7F9A).
 */
typedef struct {
    const char *blank, *magic;
    const char *label1; /* NULL = not shown */
    const char *label2;
} AlchemyPanelText;

void alchemyStatusPanelDraw(const ViewRenderer *r, const AlchemyPanelText *text, const char *name, int mp, int mpMax, const Bcd4 counter1, const Bcd4 counter2);

#endif

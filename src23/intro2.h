#ifndef YENDOR23_INTRO2_H
#define YENDOR23_INTRO2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "exedata.h"

/*
 * Chapter 2's opening story (RunCharacterCreation, yendor2.asm:8381, called by InitGame and by the title screen's 'I' key before the
 * party is created): ComposeCharacterPortrait (:9549) builds a tall backdrop, PlayCharacterCreationIntroAnimation (:8626) fades it in, and
 * RunCharacterCreationSelectionStep (:8800 -- despite its old name it is the story, no selection happens in it) pans down the backdrop
 * and shows the story cards. Escape at any poll (PollForEscapeKeyOnlyAlt) abandons the rest; FinalizeCharacterCreation (:8420) always runs.
 * Chapter 3 has a different, shorter opening (five phases of picture flip-books and fades; see docs23/engine-diffs.md) and does not use this module.
 *
 * The backdrop is two pictures of PICTURES category 0 in one buffer of 1F41h paragraphs: picture 5 at row 0 and picture 6 at row 196
 * (fe + 0xF50 paragraphs). A frame copies the 200 rows starting at row `scroll` to the screen; the story pans down 198 rows in three
 * stretches (31, 39 and 128 rows, one row per frame) separated by palette fades.
 *
 * Seven animated cells (a 20-byte record each at DS:0x6D60, filled by ComposeCharacterPortrait; all zero in the executable) are drawn over
 * the backdrop after the copy, shifted by the scroll. Fields: flags, first picture, last picture, PICTURES category index, x, width, y,
 * height, current picture. The picture is clipped against the top and bottom of the screen; a cell whose bottom has scrolled above the screen is
 * switched off. Flags: 0x8000 on, 0x4000 animating, 0x80 loop back to the first picture, 0x40 count up (without 0x80 and 0x40 the cell
 * counts down), 0x20 stop at the end (clears 0x4000, 0x40, 0x20). The cells only change picture on frames where the 0x400 timer flag is
 * set (the animation tick, whose rate is the animation speed setting); the frame clears it.
 *
 * Cell uses (viewed with tools/intro_sheet.c; categories here are PICTURES category indices, g_pictureCategory / 0x10): 0 and 1 the
 * two waving flags on the poles (category 6, pictures 0x1A-0x1F, x 72 and 240, y 94, 47 rows of the 136-row pictures), 2 and 3 the
 * two glowing wall plaques below them (category 6, 0x17-0x19, y 237), 4 the halberd guard in the middle (category 2, 0x0A-0x0F, x 105, y
 * 242), 5 the robed sorcerer raising his hands (category 2, 0x10-0x13, x 180, y 48), 6 a door opening on a silhouette (category 1,
 * 0x49-0x4E, x 86, y 46). Cells 5 and 6 start switched off; the story switches them on for the dream and the knock at the door.
 *
 * Story cards (DrawShadowedText): lines of the executable's text, each 6 rows below the previous, drawn twice -- in colour 0x93 and then one
 * pixel up-left in colour 0x98, over a black band. When the sound-effects driver flag (8) is on and the card has a voice id, the voice
 * is played instead and the text is NOT drawn (the narrated version); otherwise the text shows. Nine cards, in order of appearance:
 * see introCards.
 */
enum { IntroCellCount = 7 };

enum {
    IntroCellOn = 0x8000,
    IntroCellAnimating = 0x4000,
    IntroCellLoop = 0x80,
    IntroCellUp = 0x40,
    IntroCellOnce = 0x20
};

typedef struct {
    uint16_t flags;
    uint16_t firstPicture, lastPicture;
    uint16_t category;
    uint16_t x, width, y, height;
    uint16_t picture;
} IntroCell;

/* The table ComposeCharacterPortrait writes. */
void introCellsInit(IntroCell cells[IntroCellCount]);

typedef struct {
    uint16_t category, picture;
    int x, y;                /* screen position after the clip (y >= 0) */
    unsigned firstRow;       /* the picture row drawn first (non-zero when its top is above the screen) */
    unsigned rows;           /* how many rows are drawn from there (see the quirk in introCellsFrame) */
    unsigned width;
} IntroCellDraw;

/*
 * DrawCharacterCreationAnimationFrame's cell pass: the draw list (in cell order, at most 7) for a view scrolled by (scrollX, scrollY);
 * updates the cells -- switched off when scrolled out above the screen, and on a `tick` frame the animating ones step to their next
 * picture. Cells below the screen (y > 199) are neither drawn nor stepped. Returns the number of draws.
 * A cell cut off at the top starts at picture row (scrollY - y) but still draws its full height in rows (the original passes the
 * unreduced height), so it reads that many rows past the picture's end; one cut off at the bottom draws 200 - y rows.
 */
unsigned introCellsFrame(IntroCell cells[IntroCellCount], int scrollX, int scrollY, bool tick, IntroCellDraw draws[IntroCellCount]);

/* A story card: where its text is in the executable's data segment and how it is drawn. */
typedef struct {
    unsigned textOffset;  /* DS offset of the first line; the lines follow, NUL separated */
    unsigned lines;
    int x, y;
    unsigned voice;       /* the sound-event id played instead of the text when sound effects are on; 0xFFFF none */
} IntroCard;

enum { IntroCardCount = 9, IntroCardNoVoice = 0xFFFF, IntroCardLineMax = 64 };

/* The nine cards of Chapter 2's story, in the order they appear. */
const IntroCard *introCards(void);

/* Line `index` of the card read from the executable; false when the card is not that long or the offset is outside the file. */
bool introCardLine(const ExeData *exe, const IntroCard *card, unsigned index, char out[IntroCardLineMax]);

#endif

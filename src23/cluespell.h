#ifndef YENDOR23_CLUESPELL_H
#define YENDOR23_CLUESPELL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"
#include "viewrender.h"

/*
 * The clue book's spell page (F3 SPELLS; RunClueBookSpellCategory yendor2.asm:5283, ShowClueBookSpellDetail :6091 with its out-of-line
 * chunk, Chapter 3 the same except that the ORE cost is gone). The spell is an 80-byte record (spellrecord.h). On the usual clue backdrop
 * (screen cleared to 0, category 0 picture 13 / Chapter 3 picture 6 at (1, 1)), the spell's name at (6, 4) in 0xD, the heading SPELL
 * INFORMATION at the top right and the navigation bar:
 *
 *   (44, 28) "CLASS:    LEVEL:" in 0x0A; the cost labels (MP:, NUORE:, ORE:) one under the other from (200, 40) in 0x0A with the costs
 *            (record words 0x18, 0x1A, 0x1C as plain numbers) at x = 236 in 0x8A; the effect labels (AFFECTS:, WHEN:, EFFECT: on lines
 *            1, 3 and 5) from (44, 76), 6 pixels apart.
 *   class rows from y = 34, 6 pixels apart, only for the classes that can use the spell: the class name at x = 44 (0xD), a number at
 *            x = 104 (0xD) and a word at x = 122:
 *              the spell is one of the class's two starting abilities (chargen.h)  -> 1, "CREATION", in 0xA7
 *              the class unlocks it at an even level L (partyAbilityUnlockTable)    -> L, "TRAINING", in 0x8A
 *              else the class is in the record's eligibility mask (word 0x44, bit 0x20 for MONK down to 0x01 for MARKSMAN) -> the
 *              required level (word 0x16), "SCROLL", in 0xCA
 *   AFFECTS  at (92, 76) in 0xCA -- ALL or ONE (ALL when resist flags & 6 or flags B & 0x5E00) -- then, in 0xD from x = 116:
 *              flags B & 0xC000: CHARACTER;  else resist flags & 6 or flags B & 0x200: VISIBLE MONSTERS (VISIBLE UNDEADS when resist
 *              flags & 0x100 and the target type word 0x1E is 13) and nothing more;  else MONSTER, or INSECT / UNDEAD (target type 9 / 13
 *              with resist flags & 0x100); an "S" follows when flags B & 0x5C00; then in 0xCA the range: IN HAND TO HAND (flags B & 0x3000),
 *              IN A STRAIGHT LINE (0x800), IN A 3X3 AREA (0x400), AT A DISTANCE (0x100) or nothing.  Skipped altogether when flags B & 0xFF.
 *   WHEN     at (74, 88) in 0xA7 -- OUT OF HAND TO HAND (flags A & 0x400), IN HAND TO HAND (flags B & 0x3000) or ANYTIME.
 *   EFFECT   the description lines from (44, 106) in 0xD, 6 pixels apart (spellDescription below).
 * Labels come from the executable (the addresses are in cluespell.c).
 */
typedef struct {
    char header[24];
    char costLabels[3][8];
    unsigned costLabelCount; /* 3 in Chapter 2, 2 in Chapter 3 */
    char effectLabels[5][12];
    char classNames[6][12];
    char scroll[8], creation[12], training[12], all[6], one[6], visibleMonsters[20], visibleUndeads[20], monster[10], insect[10], undead[10],
        character[12], plural[4], handToHand[20], straight[24], area[20], distance[20], outOfHand[24], anytime[12];
} ClueSpellText;

bool clueSpellTextLoad(ClueSpellText *text, const ExeData *exe, GameKind game);

/* Description lines are 39 bytes in WORLD.DAT; `count` of them follow `first` (an index into the line block). */
enum { ClueSpellLineSize = 39 };

/* Where spell `id` (1-based)'s description lines are: reads the (first line, count) pair of the index block. False if out of range. */
bool spellDescriptionRange(GameKind game, const uint8_t *worldDat, size_t size, unsigned id, unsigned *first, unsigned *count);

/* Line `index` of the line block (a pointer into worldDat, ClueSpellLineSize bytes), or NULL. */
const uint8_t *spellDescriptionLine(GameKind game, const uint8_t *worldDat, size_t size, unsigned index);

typedef struct {
    const uint8_t *lines[16];
    unsigned lineCount;
} ClueSpellDescription;

void clueSpellPageDraw(const ViewRenderer *r, const ClueSpellText *text, const uint8_t *spellRecord, unsigned spellId, const char *heading, uint16_t navFlags,
                       const ClueSpellDescription *description);

#endif

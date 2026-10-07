#ifndef YENDOR23_CLUEMONSTER_H
#define YENDOR23_CLUEMONSTER_H

#include <stdbool.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"
#include "monster.h"
#include "viewrender.h"

/*
 * The clue book's monster statistics page (F2; ShowClueBookMonsterDetail yendor2.asm:6713, Chapter 3 the same with its own label
 * positions). Drawn over the same backdrop as the item page (clueitem.h: screen cleared to 0, category 0 picture 13 -- Chapter 3 picture 6 --
 * at (1, 1), the monster's name at (6, 4) in 0xD, the heading MONSTER STATISTICS at the top right) with 23 rows. Every row is a label in
 * colour 0x0A at its own (x, y) -- the x values right-align the labels so that the values line up -- and a value:
 *   BCD field   (4 rows: EXPERIENCE [+0x8A], GOLD [+0x7E], MAGIC ORE [+0x86], NUORE [+0x82]) the loot as a comma grouped number at x = 251 in
 *               0x8A when not zero
 *   stat        (7 rows: HEALTH +0x50, ACCURACY +0x54, DEXTERITY +0x56, ABSORPTION +0x58, DAMAGE +0x5A, RANGED ACC. +0x64, RANGED DAM. +0x66)
 *               the u16 at x = 275 in 0x59 when not zero
 *   immune      (10 rows, one per bit 0x8000 .. 0x0001 of the flag word [+0x96]; 0x0200 and 0x0100 have no row) a mark at x = 263 in 0xA7
 *   resistant   (2 rows: the label at y = 154 marks bits 0x3A00 of [+0x98], or bit 0x10 of [+0x96]; the label at y = 160 marks 0xC000 of [+0x98])
 * and, at (171 or right aligned, 169), the line of attack effects: the label SPECIAL ATTACK- in 0x0A and after it, in 0xCA, the words PARTY ATTACK (area
 * flag 0x1000) + the inflicted conditions of the special attack effect (+0x6E; effect.h costFlags bits 0x8000 .. 0x80 as SICK, POISON, DISEASE, PARALYZE, FROZEN,
 * STONING, JINXING, HEXING, CURSING, then the thefts 0x1 GOLD, 0x4 ORE (Chapter 3 FOOD), 0x2 NUORE) + for the special-corrode flags 0xE00 of [+0x92] BREAK or
 * DESTROY (modeFlags 0x200) and PROJECTILE / WEAPON / SHIELD (0x800 / 0x400 / 0x200), each word followed by ", " with the last two characters cut; with no
 * special attack (and no area flag) only the label shows. The label starts at x = (35 - length) * 6 when the words are longer than 8 characters, so the line ends
 * at the right edge, and the words follow 90 pixels later.
 * The sprite is drawn as its first idle frame (MonsterFieldSpriteBase; category 2 at (8, 7), or category 3 at (6, 33) for the alternate large layout); the
 * animation (a frame counter that cycles idle frames, then plays the attack frames and its sound) and the palette remap are not.
 * The label and mark texts come from the executable (the tables in cluemonster.c hold their data-segment addresses).
 */
enum { ClueMonsterRows = 23 };

/* The words of the attack line in the order the original appends them: 0 PARTY ATTACK, 1-9 the conditions (0x8000 .. 0x80), 10-12 the thefts (0x1, 0x4, 0x2), 13 BREAK, 14 DESTROY, 15-17 PROJECTILE / WEAPON / SHIELD. */
enum { ClueAttackWordCount = 18 };

typedef struct {
    char label[ClueMonsterRows][24];
    char immuneMark[16], resistantMark[16];
    char heading[32]; /* MONSTER STATISTICS */
    char attackLabel[24];
    char attackWord[ClueAttackWordCount][16]; /* with their trailing ", " */
} ClueMonsterText;

bool clueMonsterTextLoad(ClueMonsterText *text, const ExeData *exe, GameKind game);

/* The words of the attack line for a monster record (empty when it has no special attack and no area flag; *hasLine says whether the line has words at all). */
void clueMonsterAttackWords(const ClueMonsterText *text, GameKind game, const uint8_t *monsterRecord, char out[160], bool *hasWords);

/* The page for a monster record (MonsterRecordSize bytes) with its display name. */
void clueMonsterPageDraw(const ViewRenderer *r, const ClueMonsterText *text, const uint8_t *monsterRecord, const char *name, uint16_t navFlags);

#endif

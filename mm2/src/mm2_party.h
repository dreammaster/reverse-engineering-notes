/* Character creation and level-up rules (docs/classes.md). */
#ifndef MM2_PARTY_H
#define MM2_PARTY_H

#include "mm2_data.h"

typedef struct {
	int cls, race, alignment, sex;
	char name[12];
	uint8_t stats[7];   /* Might, Intellect, Personality, Endurance, Speed, Accuracy, Luck */
} Mm2NewChar;

/* Fills `c` the way 1MENU2 create_character_record (18624) does. */
void mm2_create_character(Mm2Char *c, const Mm2NewChar *n);

/* Racial adjustment of a stat (race 0-4, stat 0-6 in creation order): -1, 0, +1, +2. */
int mm2_race_stat_adjust(int race, int stat);

/* First free roster slot (no name) or -1. */
int mm2_roster_find_free(const Mm2Roster *r);

/* ---- training hall (2MISC2 training_hall 1CE30; docs/classes.md) ---- */
typedef enum { MM2_TRAIN_OK, MM2_TRAIN_DISABLED, MM2_TRAIN_NEED_EXP, MM2_TRAIN_NEED_GOLD } Mm2TrainResult;

/* Level the character would reach (base level + 1, unchanged at 255). */
int mm2_train_target_level(const Mm2Char *c);
uint32_t mm2_train_cost(const Mm2Char *c, int town);
uint32_t mm2_train_exp_needed(const Mm2Char *c);
Mm2TrainResult mm2_train_check(const Mm2Char *c, int town);

typedef struct {
	int hpGained;
	int newSpells;   /* a new spell level was reached and its spells added to the book */
} Mm2LevelUp;

/* Pays the cost, raises the level and applies hit points, thievery and spells.  Call only after mm2_train_check
 * returned MM2_TRAIN_OK.  Cost 0 (free training) instead gives gold/2 more gold, capped at 50000. */
Mm2LevelUp mm2_level_up(Mm2Char *c, int town);

/* Spell level progression for the classes with spells (loc_1C6CC): returns 1 if a new spell level was reached. */
int mm2_update_spell_level(Mm2Char *c);

/* Number of the character's two skill slots (char +50h nibbles) equal to `skill` (resident char_skill_count, mm2.asm 0x13664).
 * Skill ids: 1 Arms Master ... 10 Merchant ... (DGROUP:046A). */
int mm2_char_skill_count(const Mm2Char *c, int skill);
#define MM2_SKILL_MERCHANT 10

#endif

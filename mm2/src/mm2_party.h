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

#endif

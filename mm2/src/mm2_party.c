#include "mm2_party.h"
#include "mm2_tables.h"

#include <string.h>

static void put16(Mm2Char *c, int off, unsigned v) {
	c->raw[off] = (uint8_t)v;
	c->raw[off + 1] = (uint8_t)(v >> 8);
}

int mm2_race_stat_adjust(int race, int stat) {
	int v = MM2_RACE_STAT_ADJ[race * 7 + stat];
	return v == 255 ? -1 : v;
}

int mm2_roster_find_free(const Mm2Roster *r) {
	int i;
	for (i = 0; i < MM2_ROSTER_CHARS; i++)
		if (!r->chars[i].raw[MC_NAME]) return i;
	return -1;
}

void mm2_create_character(Mm2Char *c, const Mm2NewChar *n) {
	uint8_t *r = c->raw;
	int k, endurance = n->stats[3], luck = n->stats[6];
	unsigned hp;
	memset(r, 0, MM2_CHAR_SIZE);
	memcpy(r + MC_NAME, n->name, 11);
	r[MC_TOWN] = 1;
	r[MC_SEX] = (uint8_t)n->sex;
	r[MC_RACE] = (uint8_t)n->race;
	r[MC_CLASS] = (uint8_t)n->cls;
	r[MC_ALIGN] = r[MC_ORIG_ALIGN] = (uint8_t)n->alignment;
	r[MC_CUR_STATS + 0] = r[MC_BASE_STATS + 0] = n->stats[0];   /* Might */
	r[MC_CUR_STATS + 1] = r[MC_BASE_STATS + 1] = n->stats[1];   /* Intellect */
	r[MC_CUR_STATS + 2] = r[MC_BASE_STATS + 2] = n->stats[2];   /* Personality */
	r[MC_ENDURANCE] = r[MC_BASE_ENDURANCE] = n->stats[3];
	r[MC_CUR_STATS + 3] = r[MC_BASE_STATS + 3] = n->stats[4];   /* Speed */
	r[MC_CUR_STATS + 4] = r[MC_BASE_STATS + 4] = n->stats[5];   /* Accuracy */
	r[MC_CUR_STATS + 5] = r[MC_BASE_STATS + 5] = n->stats[6];   /* Luck */
	r[MC_LEVEL] = r[MC_BASE_LEVEL] = 1;
	for (k = 0; k < 8; k++)
		r[MC_RESIST + k] = MM2_RACE_RESIST[k * 6 + n->race];
	r[MC_THIEVERY] = MM2_START_THIEVERY[n->cls];
	hp = (endurance < 22 ? MM2_ENDURANCE_HP[endurance] : 0u) + MM2_START_HP[n->cls];
	put16(c, MC_HP, hp);
	put16(c, MC_HP_MAX, hp);
	put16(c, 0x60, hp);
	if (n->cls == MM2_CLERIC || n->cls == MM2_SORCERER) {
		int stat = n->cls == MM2_CLERIC ? n->stats[2] : n->stats[1];
		unsigned sp = stat < 23 ? MM2_START_SP[stat] : 0u;
		r[MC_SPELL_LEVEL] = r[MC_BASE_SPELL_LEVEL] = 1;
		put16(c, MC_SP, sp);
		put16(c, MC_SP_MAX, sp);
		r[MC_SPELL_BITS] = n->cls == MM2_CLERIC ? 0x5C : 0x3A;
	}
	r[MC_AGE] = 18;
	r[MC_AC] = n->stats[4] < 15 ? MM2_START_AC[n->stats[4]] : 0;
	r[MC_FOOD] = 10;
	if (luck >= 22 || MM2_ENDURANCE_HP[luck] == 0)
		r[MC_PACK_ID] = 1;
	else
		r[MC_PACK_ID] = MM2_START_ITEM[MM2_ENDURANCE_HP[luck] * 8 + n->cls];
	r[MC_CONDITION] = 0;
}

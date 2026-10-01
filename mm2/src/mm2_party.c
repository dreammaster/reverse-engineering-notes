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

/* TODO(review): port of ovl/1MENU2.asm create_character_record (IDA 0x18624..0x187F4).  Verified against the six shipped
 * level-1 characters, but note:
 *  - the disassembly I read lost a few lines (inline XREF comments): the load of stats[2] for +6D/+12 and the class test
 *    (Cleric or Sorcerer get spells) were reconstructed from context and from the shipped data;
 *  - the starting backpack item uses the word table at DGROUP:06F2 indexed by LUCK and the byte table at 075C
 *    (MM2_ENDURANCE_HP / MM2_START_ITEM).  The same 06F2 table is also indexed by Endurance for the starting HP.  This is
 *    what the code does (0x187A5..0x187E5) but it looks odd: please confirm the second use is really Luck. */
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

/* ---- training ---- */
int mm2_train_target_level(const Mm2Char *c) {
	int l = (int)mm2_c8(c, MC_BASE_LEVEL);
	return l == 0xFF ? l : l + 1;
}

uint32_t mm2_train_cost(const Mm2Char *c, int town) {
	return mm2_training_cost(town, mm2_train_target_level(c));
}

uint32_t mm2_train_exp_needed(const Mm2Char *c) {
	return mm2_exp_for_level((int)mm2_c8(c, MC_CLASS), mm2_train_target_level(c));
}

Mm2TrainResult mm2_train_check(const Mm2Char *c, int town) {
	if (mm2_c8(c, MC_CONDITION) != 0) return MM2_TRAIN_DISABLED;
	if (mm2_c32(c, MC_EXP) < mm2_train_exp_needed(c)) return MM2_TRAIN_NEED_EXP;
	if (mm2_c32(c, MC_GOLD) < mm2_train_cost(c, town)) return MM2_TRAIN_NEED_GOLD;
	return MM2_TRAIN_OK;
}

static void put32(Mm2Char *c, int off, uint32_t v) {
	int i;
	for (i = 0; i < 4; i++)
		c->raw[off + i] = (uint8_t)(v >> (8 * i));
}

static int bracket_or_zero(int stat) {
	int b = mm2_bracket(stat);
	return (uint8_t)b >= 0xF0 ? 0 : b;   /* negative brackets count as 0 (cmp al, F0h) */
}

/* TODO(review): assumptions here, compare with ovl/2MISC2.asm, the routine at loc_1C6CC (IDA 0x1C6CC..0x1C85C, called from
 * the training hall's level-up code at ~0x1CA66):
 *  - casting stat: the original reads char +12h (Personality) by default and +11h (Intellect) when var_14 is set, which
 *    it sets for classes 1 and 2 only (Paladin, Archer).  Taken literally that gives Sorcerers Personality, contradicting
 *    character creation (Intellect) and the shipped Sorcerers.  I used Cleric/Paladin = Personality, Sorcerer/Archer =
 *    Intellect instead; the original's choice is unverified.
 *  - maximum SP = spellLevel * (bracket(stat) + 3) as in the multiply at ~0x1C85C.  The premade characters in ROSTER.DAT
 *    do NOT follow this (they look like baseLevel * (bracket + 3)), so the formula is unconfirmed.
 *  - the "cap reached" branch at loc_1C7F4 sets the spell level used for SP to the character level (+20); copied as is, odd.
 *  - hybrids (Paladin/Archer) use level-6 and stop advancing at spell level 8: from the compares at 0x1C6F4..0x1C767. */
int mm2_update_spell_level(Mm2Char *c) {
	int cls = (int)mm2_c8(c, MC_CLASS);
	int hybrid = cls == MM2_PALADIN || cls == MM2_ARCHER;
	int cur = (int)mm2_c8(c, MC_BASE_SPELL_LEVEL), level = (int)mm2_c8(c, MC_BASE_LEVEL);
	int eff = level, advance = 1, newLevel = cur, stat, mult;
	if (hybrid) {
		eff -= 6;
		if ((uint8_t)eff >= 0xF0 || eff < 0) eff = 0;
		if (level < 6) advance = 0;
	}
	if (advance) {
		int t = (eff + 1) >> 1;
		if (t <= cur || t >= 10) advance = 0;
		else newLevel = t;
		if (advance && hybrid && newLevel >= 8) advance = 0;
	}
	if (advance) {
		const uint8_t *tab = (cls == MM2_CLERIC || cls == MM2_PALADIN) ? MM2_LEARN_CLERIC : MM2_LEARN_SORCERER;
		int i;
		c->raw[MC_BASE_SPELL_LEVEL] = c->raw[MC_SPELL_LEVEL] = (uint8_t)newLevel;
		if (newLevel >= 1 && newLevel <= 8)
			for (i = 0; i < 4; i++) {
				int code = tab[(newLevel - 1) * 4 + i];
				if (code == 0x80) continue;
				if (code > 0x2F) code -= 0x30;
				c->raw[MC_SPELL_BITS + (code >> 3)] |= (uint8_t)(1 << (code & 7));
			}
	} else {
		int cap = hybrid ? 8 : 9;
		if (cur >= cap) newLevel = level;   /* as in the original: the maximum uses the character level */
	}
	/* maximum spell points = spell level x (bracket of the casting stat + 3); Cleric/Paladin use Personality,
	 * Sorcerer/Archer Intellect */
	stat = (int)mm2_c8(c, (cls == MM2_CLERIC || cls == MM2_PALADIN) ? MC_BASE_STATS + 2 : MC_BASE_STATS + 1);
	mult = bracket_or_zero(stat) + 3;
	{
		unsigned sp = (unsigned)(newLevel * mult);
		c->raw[MC_SP_MAX] = c->raw[MC_SP] = (uint8_t)sp;
		c->raw[MC_SP_MAX + 1] = c->raw[MC_SP + 1] = (uint8_t)(sp >> 8);
	}
	return advance;
}

/* TODO(review): ovl/2MISC2.asm, the code after the "Sorry - you need more gold" check (IDA ~0x1C960..0x1CA72).  The hit point
 * gain (HPperLevel * townMult / townDiv, rounded up unless Cleric/Ninja/Robber, plus the endurance bracket, negative
 * brackets treated as 0) was read from the code; the "free training" special case (cost 0: gold += gold/2, max 50000,
 * at ~0x1C8F4) is copied but I did not work out when the cost can be 0. */
Mm2LevelUp mm2_level_up(Mm2Char *c, int town) {
	Mm2LevelUp r = {0, 0};
	int cls = (int)mm2_c8(c, MC_CLASS);
	uint32_t cost = mm2_train_cost(c, town), gold = mm2_c32(c, MC_GOLD);
	unsigned base, rem, hp;
	if (cost)
		gold -= cost;
	else {
		gold += gold / 2;
		if (gold > 50000) gold = 50000;
	}
	put32(c, MC_GOLD, gold);
	if (c->raw[MC_BASE_LEVEL] != 0xFF) c->raw[MC_BASE_LEVEL]++;
	if (c->raw[MC_LEVEL] != 0xFF) c->raw[MC_LEVEL]++;
	if ((cls == MM2_NINJA || cls == MM2_ROBBER) && c->raw[MC_THIEVERY] && c->raw[MC_THIEVERY] != 0xFF)
		c->raw[MC_THIEVERY]++;
	base = (unsigned)MM2_HP_PER_LEVEL[cls] * MM2_TOWN_MULT[town];
	rem = base % MM2_TOWN_HP_DIV[town];
	base /= MM2_TOWN_HP_DIV[town];
	if (cls == MM2_CLERIC || cls == MM2_NINJA || cls == MM2_ROBBER) rem = 0;
	if (rem) base++;
	hp = base + (unsigned)bracket_or_zero((int)mm2_c8(c, MC_BASE_ENDURANCE));
	put16(c, 0x60, mm2_c16(c, 0x60) + hp);
	put16(c, MC_HP_MAX, mm2_c16(c, MC_HP_MAX) + hp);
	put16(c, MC_HP, mm2_c16(c, MC_HP) + hp);
	r.hpGained = (int)hp;
	if (cls >= MM2_PALADIN && cls <= MM2_SORCERER)
		r.newSpells = mm2_update_spell_level(c);
	return r;
}

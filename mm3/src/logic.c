#include "logic.h"

#include <string.h>
#include "rules.h"

static unsigned rd16(const Mm3Game *g, unsigned off) { return g->dg[off] | (g->dg[off + 1] << 8); }

Mm3Character *mm3_party_member(const Mm3Game *g, unsigned index) {
	return (Mm3Character *)(g->dg + MM3_DG_PARTY_CHARS + index * sizeof(Mm3Character));
}

/* Experience needed for level `level`: levels 2..11 grow by a class-dependent base doubling each level (base << (level - 2)), from level 12 on
 * the requirement grows by a fixed 0xFA000 (1,024,000) per level on top of the level-12 value. */
static uint32_t level_threshold(const Mm3Game *g, const Mm3Character *ch, unsigned level) {
	unsigned linear, shift;
	if (level < 12) { linear = 0; shift = level - 1; }
	else { linear = level - 11; shift = 10; }
	int32_t base = (int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass);
	return (uint32_t)((int32_t)linear * 0xFA000 + (base << shift));
}

uint32_t mm3_experience_total(const Mm3Game *g, const Mm3Character *ch) {
	if (ch->level - 1 == 0) return ch->experience;                       /* level 1: just the stored experience */
	unsigned level = ch->level - 1; /* the original works with level - 1 here */
	uint32_t base;
	if (level < 12) base = (uint32_t)((int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass) << (level - 1));
	else base = (uint32_t)((int32_t)(level - 11) * 0xFA000 + ((int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass) << 10));
	return base + ch->experience;
}

uint32_t mm3_experience_for_next_level(const Mm3Game *g, const Mm3Character *ch) {
	return level_threshold(g, ch, ch->level);
}

uint32_t mm3_experience_needed(const Mm3Game *g, const Mm3Character *ch) {
	int32_t need = (int32_t)(mm3_experience_for_next_level(g, ch) - mm3_experience_total(g, ch));
	return need > 0 ? (uint32_t)need : 0;
}

void mm3_give_experience(const Mm3Game *g, uint32_t amount) {
	unsigned count = g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_PARTY_SIZE] : g->dg[MM3_DG_PARTY_STATE_BASE];
	if (!count) return;
	uint32_t share = amount / count;
	for (unsigned i = 0; i < count; i++) {
		unsigned index = g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_ORDER + i] : i;
		mm3_party_member(g, index)->experience += share;
	}
}

/* ---- maze */
static unsigned slot_base(unsigned slot) { return MM3_DG_MAZE_PAGES + slot * 0x340; }
static unsigned slot_id(const Mm3Game *g, unsigned slot) { return g->dg[MM3_DG_MAZE_SLOT_IDS + slot]; }

int mm3_maze_neighbour_slot(const Mm3Game *g, unsigned map_id) {
	for (unsigned s = 0; s < 4; s++) if (slot_id(g, s) == map_id) return (int)s;
	return MM3_NO_SLOT;
}

/* Party-relative (x, y) -> window coordinates 0..31 and the slot holding that cell: the window is the 2x2 block of pages; slot 0 is the
 * north-west one and the other pages are found through the neighbour map ids in slot 0's header (south, then east).  Returns 0 when the cell is
 * outside the window or its page is not loaded. */
static int locate(const Mm3Game *g, int x, int y, unsigned *slot, int *cx, int *cy, int signed_range) {
	unsigned cur = (int8_t)g->dg[MM3_DG_MAZE_CUR_SLOT];
	x += g->dg[MM3_DG_MAZE_SLOT_X + cur]; y += g->dg[MM3_DG_MAZE_SLOT_Y + cur];
	if (signed_range ? (x < 0 || y < 0 || x > 31 || y > 31) : ((unsigned)x > 31 || (unsigned)y > 31)) return 0;
	unsigned s = 0;
	if (y > 15) { int n = mm3_maze_neighbour_slot(g, g->dg[slot_base(s) + 0x308]); if (n == MM3_NO_SLOT) return 0; s = (unsigned)n; }
	if (x > 15) { int n = mm3_maze_neighbour_slot(g, g->dg[slot_base(s) + 0x309]); if (n == MM3_NO_SLOT) return 0; s = (unsigned)n; }
	*slot = s; *cx = x; *cy = y;
	return 1;
}

unsigned mm3_maze_word_wrapped(const Mm3Game *g, int x, int y, unsigned mask) {
	unsigned slot; int cx, cy;
	if (!locate(g, x, y, &slot, &cx, &cy, 1)) return g->dg[MM3_DG_MAZE_WRAP_MODE] ? 0 : MM3_NO_SLOT;
	return rd16(g, slot_base(slot) + (cy & 15) * 32 + (cx & 15) * 2) & mask;
}

unsigned mm3_maze_word(const Mm3Game *g, int x, int y, unsigned mask) {
	unsigned slot; int cx, cy;
	if (!locate(g, x, y, &slot, &cx, &cy, 1)) return g->dg[MM3_DG_MAZE_WRAP_MODE] ? 0 : MM3_NO_SLOT;
	if (g->dg[MM3_DG_MAZE_WRAP_MODE]) {
		/* maps 45-48 and 49-52 are neighbouring outdoor areas that do not connect across their shared border */
		unsigned here = slot_id(g, (unsigned)(int8_t)g->dg[MM3_DG_MAZE_CUR_SLOT]), there = slot_id(g, slot);
		if (here >= 45 && here <= 48 && there >= 49 && there <= 52) return 0;
		if (here >= 49 && here <= 52 && there <= 48) return 0;
	}
	return rd16(g, slot_base(slot) + (cy & 15) * 32 + (cx & 15) * 2) & mask;
}

unsigned mm3_maze_flags(const Mm3Game *g, int x, int y, unsigned mask) {
	unsigned slot; int cx, cy;
	if (!locate(g, x, y, &slot, &cx, &cy, 1)) return g->dg[MM3_DG_MAZE_WRAP_MODE] ? 0 : MM3_NO_SLOT;
	return g->dg[slot_base(slot) + 0x200 + (cy & 15) * 16 + (cx & 15)] & mask;
}

void mm3_maze_set_bits(const Mm3Game *g, int x, int y, unsigned field, unsigned value) {
	unsigned slot = (unsigned)(int8_t)g->dg[MM3_DG_MAZE_CUR_SLOT];
	if (y > 15) slot = (unsigned)mm3_maze_neighbour_slot(g, g->dg[slot_base(slot) + 0x308]);
	if (x > 15) slot = (unsigned)mm3_maze_neighbour_slot(g, g->dg[slot_base(slot) + 0x309]);
	unsigned at = slot_base(slot) + (y & 15) * 32 + (x & 15) * 2;
	unsigned word = rd16(g, at) & rd16(g, MM3_DG_BITSET_MASKS + field * 2);
	word |= (value << g->dg[MM3_DG_BITSET_SHIFTS + field * 0x58]) & 0xFFFF;
	g->dg[at] = (uint8_t)word; g->dg[at + 1] = (uint8_t)(word >> 8);
}

void mm3_set_bit(uint8_t *bits, unsigned index, int value) {
	unsigned mask = 0x80u >> (index & 7);
	if (value) bits[index >> 3] |= (uint8_t)mask; else bits[index >> 3] &= (uint8_t)~mask;
}

unsigned mm3_is_bit_set(const uint8_t *bits, unsigned index) {
	return (unsigned)(uint16_t)((int16_t)(int8_t)bits[index >> 3] & (0x80u >> (index & 7)));
}

void mm3_maze_mark_visited(const Mm3Game *g, int x, int y) {
	unsigned slot; int cx, cy;
	if (!locate(g, x, y, &slot, &cx, &cy, 1)) return;
	mm3_set_bit(g->dg + slot_base(slot) + 0x320, (cy & 15) * 16 + (cx & 15), 1);
}

unsigned mm3_maze_is_visited(const Mm3Game *g, int x, int y) {
	unsigned slot; int cx, cy;
	if (!locate(g, x, y, &slot, &cx, &cy, 0)) return 0;
	return mm3_is_bit_set(g->dg + slot_base(slot) + 0x320, (cy & 15) * 16 + (cx & 15));
}

/* ---- conditions and damage */
unsigned mm3_worst_condition(const Mm3Character *ch) {
	for (int i = 15; i >= 0; i--) if (ch->conditions[i]) return (unsigned)i;
	return 16;
}

static unsigned party_count(const Mm3Game *g) {
	return g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_PARTY_SIZE] : g->dg[MM3_DG_PARTY_STATE_BASE];
}
static const Mm3Character *member_in_order(const Mm3Game *g, unsigned i) { /* party member of slot i (combat order in combat) */
	return mm3_party_member(g, g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_ORDER + i] : i);
}

/* conditions 11-15 (paralysed ... eradicated) take a character out; the party is "dead" when everyone is out */
void mm3_check_party_dead(const Mm3Game *g) {
	for (unsigned i = 0; i < party_count(g); i++) {
		unsigned w = mm3_worst_condition(member_in_order(g, i));
		if (w <= 10 || w == 16) { g->dg[MM3_DG_PARTY_DEAD_FLAG] = 0; return; }
	}
	g->dg[MM3_DG_PARTY_DEAD_FLAG] = 1;
}

int mm3_all_have_gone(const Mm3Game *g) {
	unsigned groups = (g->dg[MM3_DG_MONSTER_ROWS] > 0) + (g->dg[MM3_DG_MONSTER_ROWS + 1] > 0) + (g->dg[MM3_DG_MONSTER_ROWS + 2] > 0);
	unsigned party = g->dg[MM3_DG_COMBAT_PARTY_SIZE];
	for (unsigned i = 0; i < party + groups; i++) {
		if (g->dg[MM3_DG_COMBAT_GONE + i]) continue;
		if (i >= party) return 0;                                   /* a monster group still to move */
		unsigned w = mm3_worst_condition(mm3_party_member(g, g->dg[MM3_DG_COMBAT_ORDER + i]));
		if (w < 11 || w > 15) return 0;                            /* an able character that has not acted */
	}
	return 1;
}

int mm3_chars_cant_act(const Mm3Game *g) {
	for (unsigned i = 0; i < g->dg[MM3_DG_COMBAT_PARTY_SIZE]; i++) {
		unsigned w = mm3_worst_condition(mm3_party_member(g, g->dg[MM3_DG_COMBAT_ORDER + i]));
		if (w != 8 && !(w >= 11 && w <= 15)) return 0;
	}
	return 1;
}

void mm3_subtract_hit_points(const Mm3Game *g, Mm3Character *ch, int amount) {
	Mm3Rules rules = { g->dg };
	ch->hp = (int16_t)(ch->hp - amount);
	int dead = ch->hp <= -10;
	if (ch->hp < 1) {
		if ((int16_t)(ch->hp + (int16_t)mm3_max_hp(&rules, ch)) < 1) { /* at or below minus max hp: dead */
			ch->conditions[0x0D] = 1; /* offset 120h */
			dead = 1;
		} else {
			ch->conditions[0x0C] = 1; /* 11Fh: unconscious */
		}
		if (dead) { /* a death breaks the first worn armour piece */
			for (int i = 0; i < 18; i++)
				if (ch->slotId[i] > 0x21 && ch->slotId[i] < 0x2A && ch->slotPresent[i]) { ch->slotFlags[i] |= 0x80; break; }
		}
	}
}

/* ---- combat rolls */
extern int mm3_rnd(int lo, int hi); /* the game's random number routine (host_rnd) */

void mm3_weapon_damage(const Mm3Game *g, const Mm3Character *ch, int ranged) {
	uint8_t *dg = g->dg;
	dg[MM3_DG_WEAPON_SIDES] = dg[MM3_DG_WEAPON_DICE] = dg[MM3_DG_COMBAT_HIT_BONUS] = dg[MM3_DG_WEAPON_SPELL] = dg[MM3_DG_WEAPON_ELEMENT] = 0;
	int damage = 0;
	for (int i = 0; i < 18; i++) {
		int weapon = ranged ? ch->slotPresent[i] == 4 : (ch->slotPresent[i] == 1 || ch->slotPresent[i] == 0xD);
		if (!weapon) continue;
		dg[MM3_DG_WEAPON_ELEMENT] = ch->slotElement[i];
		dg[MM3_DG_WEAPON_SPELL] = ch->slotSpell[i];
		dg[MM3_DG_COMBAT_HIT_BONUS] = (uint8_t)(dg[MM3_DG_WEAPON_HIT_BONUS + ch->slotMetal[i]] + ch->heroism);
		damage = (int8_t)dg[MM3_DG_WEAPON_METAL_DAMAGE + ch->slotMetal[i]] + ch->holyBonus;
		dg[MM3_DG_WEAPON_DICE] = dg[MM3_DG_WEAPON_DICE_COUNT + ch->slotId[i]];
		dg[MM3_DG_WEAPON_SIDES] = dg[MM3_DG_WEAPON_DICE_SIDES + ch->slotId[i]];
		for (int d = 0; d < (int8_t)dg[MM3_DG_WEAPON_DICE]; d++) damage += mm3_rnd(1, (int8_t)dg[MM3_DG_WEAPON_SIDES]);
	}
	if (damage < 1) damage = 0;
	dg[MM3_DG_COMBAT_WEAPON_DAMAGE] = (uint8_t)damage; dg[MM3_DG_COMBAT_WEAPON_DAMAGE + 1] = (uint8_t)((unsigned)damage >> 8);
}

int mm3_hit_monster(const Mm3Game *g, const Mm3Character *ch, int ranged) {
	Mm3Rules rules = { g->dg };
	mm3_weapon_damage(g, ch, ranged);
	int roll = mm3_stat_bonus(&rules, (uint16_t)mm3_char_stat(&rules, ch, MM3_STAT_ACCURACY, 0)) + (int8_t)g->dg[MM3_DG_COMBAT_HIT_BONUS];
	static const uint8_t level_divisor[10] = { 1, 2, 2, 3, 4, 2, 2, 1, 3, 2 }; /* by class: fighters add more of their level */
	roll += mm3_char_level(ch) / level_divisor[ch->charClass % 10];
	int die;
	do { die = mm3_rnd(1, 20); roll += die; } while (die == 20);   /* a natural 20 rolls again */
	roll -= ch->conditions[0];                                      /* cursed */
	unsigned target = g->dg[MM3_DG_COMBAT_TARGET];
	if (rd16(g, MM3_DG_MON_ASLEEP + target * 2)) roll += 20;
	const uint8_t *ac_column = g->mem + (unsigned)rd16(g, MM3_DG_MON_AC + 2) * 16 + rd16(g, MM3_DG_MON_AC);
	int armour = 10 + ac_column[(uint16_t)rd16(g, MM3_DG_MON_COLUMN_OFFSET + target * 2)];
	return roll >= armour;
}

int mm3_saving_throw(const Mm3Game *g, const Mm3Character *ch, int kind) {
	Mm3Rules rules = { g->dg };
	static const struct { uint8_t scan; uint16_t offset; } RESIST[7] = { { 0, 0 }, { 0x10, 0x111 }, { 0x0B, 0x107 }, { 0x0C, 0x10B }, { 0x0D, 0x109 }, { 0x0E, 0x10D }, { 0x0F, 0x10F } };
	int chance, limit;
	if (kind == 0) { /* luck: level + twice the luck bonus, out of that + 20 */
		chance = mm3_char_level(ch) + mm3_stat_bonus(&rules, (uint16_t)(mm3_char_stat(&rules, ch, MM3_STAT_LUCK, 0) * 2));
		limit = chance + 20;
	} else { /* resistance: temporary + permanent + equipment, out of that + 40 */
		const uint8_t *r = (const uint8_t *)ch + RESIST[kind].offset;
		chance = r[0] + mm3_item_scan(&rules, ch, RESIST[kind].scan) + r[1];
		limit = chance + 40;
	}
	return mm3_rnd(1, limit) <= chance;
}

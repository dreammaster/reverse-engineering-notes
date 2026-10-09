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

/* ---- character creation */
void mm3_check_classes(const int8_t s[7], uint8_t out[10]) {
	enum { MIGHT, INT, PERS, END, SPEED, ACC, LUCK };
	out[0] = s[MIGHT] >= 15;                                                         /* knight */
	out[1] = s[MIGHT] >= 13 && s[PERS] >= 13 && s[END] >= 13;                        /* paladin */
	out[2] = s[INT] >= 13 && s[ACC] >= 13;                                           /* archer */
	out[3] = s[PERS] >= 13;                                                          /* cleric */
	out[4] = s[INT] >= 13;                                                           /* sorcerer */
	out[5] = s[LUCK] >= 13;                                                          /* robber */
	out[6] = s[SPEED] >= 13 && s[ACC] >= 13;                                         /* ninja */
	out[7] = s[END] >= 15;                                                           /* barbarian */
	out[8] = s[INT] >= 15 && s[PERS] >= 15;                                          /* druid */
	out[9] = s[INT] >= 12 && s[PERS] >= 12 && s[END] >= 12 && s[SPEED] >= 12;        /* ranger */
}

void mm3_roll_attributes(int8_t stats[7], uint8_t available[10]) {
	for (int i = 0; i < 7; i++) stats[i] = 0;
	for (int round = 0; round < 3; round++)
		for (int i = 0; i < 7; i++) stats[i] = (int8_t)(stats[i] + (mm3_rnd(10, 79) & 0xFF) / 10);
	mm3_check_classes(stats, available);
}

int mm3_thievery(const Mm3Game *g, const Mm3Character *ch) {
	Mm3Rules rules = { g->dg };
	int chance = mm3_char_level(ch) * 2;
	if (ch->charClass == 6) chance += 15;        /* ninja */
	else if (ch->charClass == 5) chance += 30;   /* robber */
	if (ch->race == 1 || ch->race == 3) chance += 10;
	else if (ch->race == 2) chance += 5;
	else if (ch->race == 4) chance -= 10;
	chance += mm3_item_scan(&rules, ch, 10);
	if (!ch->skills[0]) chance = 0;              /* thievery skill */
	chance = (int16_t)chance;
	return chance < 1 ? 0 : chance;
}

/* ---- items */
uint32_t mm3_item_price(const Mm3Game *g, const Mm3Character *ch, int slot, int mode, int discount) {
	enum { DG_BASE_PRICE = 0xB16, DG_METAL_PRICE = 0xAB6, DG_ELEMENT_PRICE = 0xA4C, DG_ATTRIBUTE_PRICE = 0xACD, DG_SPELL_PRICE = 0xDAB, DG_DISCOUNT = 0x5B15 };
	unsigned id = ch->slotId[slot];
	if (id > 0x49 && id != 0x4B && id != 0x52) return 0;       /* only equipment and the two special items have a price */
	int broken_sale = (int16_t)discount > 0x80 && mode == 2 ? 1 : (discount & 0x80);
	discount &= 0x7F;
	uint32_t base = id == 0x4B ? 2000 : id == 0x52 ? 1000 : (uint32_t)rd16(g, DG_BASE_PRICE + id * 2);
	switch (ch->slotMetal[slot]) {   /* material: cheap metals divide the price, the others multiply it */
	case 1: base /= 10; break;
	case 2: base >>= 2; break;
	case 3: base >>= 1; break;
	case 4: base -= base >> 2; break;
	default: base = (uint32_t)((int32_t)base * (int8_t)g->dg[DG_METAL_PRICE + ch->slotMetal[slot]]); break;
	}
	uint32_t element = (uint32_t)(int32_t)(int16_t)(g->dg[DG_ELEMENT_PRICE + ch->slotElement[slot]] * 100);
	uint32_t attribute = (uint32_t)(int32_t)(int16_t)((int8_t)g->dg[DG_ATTRIBUTE_PRICE + ch->slotAttribute[slot]] * 100);
	uint32_t spell = rd16(g, DG_SPELL_PRICE + ch->slotSpell[slot] * 2);
	switch (mode) {
	case 1: case 2: {
		if ((ch->slotFlags[slot] & 0xC0) && broken_sale) base = element = attribute = spell = 0;  /* cursed or broken items sell for nothing */
		uint32_t total = base + element + attribute + spell, divisor = g->dg[DG_DISCOUNT + discount];
		return divisor ? total / divisor : 0xFFFFFFFFu;
	}
	case 3: case 4: case 5: case 6: return ch->slotFlags[slot] & 0x3F;
	default: return 0;
	}
}

/* ---- monsters */
static unsigned far_column(const Mm3Game *g, unsigned pointer_at, unsigned target) { /* value of the current target in a loaded monster stat column */
	const uint8_t *col = g->mem + (unsigned)rd16(g, pointer_at + 2) * 16;
	return col[(uint16_t)(rd16(g, pointer_at) + rd16(g, MM3_DG_MON_COLUMN_OFFSET + target * 2))];
}

int mm3_monster_resistance(const Mm3Game *g, int kind) {
	unsigned target = g->dg[MM3_DG_COMBAT_TARGET], column = 0, percent = 0;
	int amount = 0;
	if (kind == 0 || kind == 3) {  /* attack with the wielded weapon: its element picks the column */
		int8_t element = (int8_t)g->dg[MM3_DG_WEAPON_ELEMENT];
		amount = g->dg[MM3_DG_ELEMENT_RESIST_BASE + (uint8_t)element];
		if (element) {
			column = element < 9 ? MM3_DG_MON_FIRE : element < 0x10 ? MM3_DG_MON_ELEC : element < 0x15 ? MM3_DG_MON_COLD
			       : element < 0x1A ? MM3_DG_MON_ACID : element < 0x22 ? MM3_DG_MON_ENER : MM3_DG_MON_MAGI;
			percent = far_column(g, column, target);
		}
	} else {                         /* spell attack: its type picks the column (and the amount stays 0) */
		static const unsigned columns[7] = { MM3_DG_MON_PHYS, MM3_DG_MON_MAGI, MM3_DG_MON_FIRE, MM3_DG_MON_ELEC, MM3_DG_MON_COLD, MM3_DG_MON_ACID, MM3_DG_MON_ENER };
		unsigned type = rd16(g, MM3_DG_SPELL_ATTACK_TYPE);
		if (type < 7) percent = far_column(g, columns[type], target);
	}
	if (percent == 0) return amount;
	if (percent == 100) return 0;
	return (int16_t)((int16_t)((100 - (int)percent) * amount) / 100);
}

int mm3_spend_spell_cost(const Mm3Game *g, Mm3Character *ch, int spell) {
	int gems = (int16_t)rd16(g, MM3_DG_SPELL_GEM_COST + spell * 2);
	int sp = (int16_t)rd16(g, MM3_DG_SPELL_SP_COST + spell * 2);
	if (sp < 1) sp = (int16_t)(mm3_char_level(ch) * (int16_t)-sp);   /* "per level" costs are stored negated */
	if (ch->sp < sp) return 1;
	uint32_t have = (uint32_t)rd16(g, MM3_DG_PARTY_GEMS) | ((uint32_t)rd16(g, MM3_DG_PARTY_GEMS + 2) << 16), need = (uint32_t)(int32_t)gems;
	if (need > have) return 2;
	ch->sp = (int16_t)(ch->sp - sp);
	have -= need;
	g->dg[MM3_DG_PARTY_GEMS] = (uint8_t)have; g->dg[MM3_DG_PARTY_GEMS + 1] = (uint8_t)(have >> 8);
	g->dg[MM3_DG_PARTY_GEMS + 2] = (uint8_t)(have >> 16); g->dg[MM3_DG_PARTY_GEMS + 3] = (uint8_t)(have >> 24);
	return 0;
}

void mm3_move_monster_by(const Mm3Game *g, int dx, int dy, int monster) {
	uint8_t *dg = g->dg;
	unsigned y = (uint16_t)(rd16(g, MM3_DG_MON_Y + monster * 2) + dy), x = (uint16_t)(rd16(g, MM3_DG_MON_X + monster * 2) + dx);
	unsigned size = dg[MM3_DG_MON_SIZE + rd16(g, MM3_DG_MON_COLUMN_OFFSET + monster * 2)];
	unsigned target_cell = MM3_DG_MON_GRID + ((y << 5) + x);
	if ((int)(dg[target_cell] + size) >= 4) return;                  /* the cell is full */
	if (rd16(g, MM3_DG_MON_ASLEEP + monster * 2) == 0 && dg[MM3_DG_MONSTERS_MOVE_FLAG]) {
		unsigned from = MM3_DG_MON_GRID + (((unsigned)rd16(g, MM3_DG_MON_Y + monster * 2) << 5) + rd16(g, MM3_DG_MON_X + monster * 2));
		dg[target_cell] = (uint8_t)(dg[target_cell] + size);
		dg[from & 0xFFFF] = (uint8_t)(dg[from & 0xFFFF] - size);
		dg[MM3_DG_MON_Y + monster * 2] = (uint8_t)y; dg[MM3_DG_MON_Y + monster * 2 + 1] = (uint8_t)(y >> 8);
		dg[MM3_DG_MON_X + monster * 2] = (uint8_t)x; dg[MM3_DG_MON_X + monster * 2 + 1] = (uint8_t)(x >> 8);
		dg[MM3_DG_MON_MOVED + monster] = 1;
	}
	dg[MM3_DG_MONSTERS_SEEN_FLAG] = 1;
}

/* Monsters near the party act in a window of 7 columns x 7 rows around it.  Each awake monster steps one cell towards the party: diagonally when the
 * walls on both axes are open, straight when only one is, not at all when both are blocked; "along"/"across" depend on which way the party faces. */
void mm3_move_monsters(const Mm3Game *g) {
	uint8_t *dg = g->dg;
	dg[MM3_DG_MOVE_ATTACKED] = 0;
	if (dg[MM3_DG_MOVE_BLOCKED]) { dg[MM3_DG_MOVE_SKIPPED] = 1; return; }
	memset(dg + MM3_DG_MON_GRID, 0, 0x400);
	memset(dg + 0xA75E, 0, 12); memset(dg + 0xAD63, 0, 12);
	memset(dg + MM3_DG_MON_MOVED, 0, 0xAA); memset(dg + MM3_DG_MON_SHOT, 0, 0xAA);
	memset(dg + 0xC4D8, 0xFF, 0x24);
	dg[MM3_DG_MONSTERS_SEEN_FLAG] = 0;
	int px = dg[MM3_DG_PARTY_X], py = dg[MM3_DG_PARTY_Y], facing = dg[MM3_DG_PARTY_FACING];
	unsigned count = dg[MM3_DG_MAZE_MONSTER_COUNT];
	for (unsigned m = 0; m < count; m++)                                    /* occupancy of every cell */
		if (rd16(g, MM3_DG_MON_Y + m * 2) < 32) {
			unsigned cell = MM3_DG_MON_GRID + ((rd16(g, MM3_DG_MON_Y + m * 2) << 5) + rd16(g, MM3_DG_MON_X + m * 2));
			dg[cell & 0xFFFF] = (uint8_t)(dg[cell & 0xFFFF] + dg[MM3_DG_MON_SIZE + rd16(g, MM3_DG_MON_COLUMN_OFFSET + m * 2)]);
		}
	for (int pass = 0; pass < 2; pass++) {
		int k = -1;
		for (int row = 3; row > -4; row--)
			for (int col = -3; col < 4; col++) {
				k++;
				unsigned cx = (uint16_t)(px + col), cy = (uint16_t)(py + row);
				for (unsigned m = 0; m < count; m++) {
					if (rd16(g, MM3_DG_MON_Y + m * 2) != cy || rd16(g, MM3_DG_MON_X + m * 2) != cx) continue;
					if (rd16(g, MM3_DG_MON_ACTIVE + m * 2) == 0 && dg[MM3_DG_ENGINE_MODE] != 5) continue;
					if (dg[MM3_DG_MON_MOVED + m]) continue;
					/* a monster in line with the party that has a ranged attack uses it (once) */
					if ((px == (int)cx || py == (int)cy) && g->mem[(unsigned)rd16(g, MM3_DG_MON_RANGED + 2) * 16 + (uint16_t)(rd16(g, MM3_DG_MON_RANGED) + rd16(g, MM3_DG_MON_COLUMN_OFFSET + m * 2))] != 0
					    && !dg[MM3_DG_MON_SHOT + m] && m + 1 != dg[MM3_DG_MONSTER_ROWS] && m + 1 != dg[MM3_DG_MONSTER_ROWS + 1] && m + 1 != dg[MM3_DG_MONSTER_ROWS + 2]
					    && rd16(g, MM3_DG_MON_ASLEEP + m * 2) == 0) {
						g->hooks->monster_ranged_attack(rd16(g, MM3_DG_MON_COLUMN_OFFSET + m * 2), cx, cy);
						dg[MM3_DG_MON_SHOT + m] = 1;
					}
					int axis = facing >> 1;
					if (axis > 1) continue;
					unsigned along = rd16(g, MM3_DG_WALL_MASKS + dg[MM3_DG_STEP_WALL_ALONG + k] * 2), across = rd16(g, MM3_DG_WALL_MASKS + dg[MM3_DG_STEP_WALL_ACROSS + k] * 2);
					int blocked_along = mm3_maze_word(g, (int16_t)cx, (int16_t)cy, along) != 0, blocked_across = mm3_maze_word(g, (int16_t)cx, (int16_t)cy, across) != 0;
					int step; /* 0 none, 1 diagonal, 2 straight */
					if (axis == 0) step = !blocked_along ? 1 : (blocked_across ? 0 : 2);
					else step = !blocked_across ? 2 : (blocked_along ? 0 : 1);
					if (step == 1) mm3_move_monster_by(g, (int16_t)rd16(g, MM3_DG_STEP_DX + k * 2), (int16_t)rd16(g, MM3_DG_STEP_DY + k * 2), (int)m);
					else if (step == 2) {
						int16_t d = (int16_t)rd16(g, MM3_DG_STEP_STRAIGHT + k * 2);
						if (k < 0x15 || k > 0x1B) mm3_move_monster_by(g, 0, d, (int)m);   /* the rows nearest the party step sideways ... */
						else mm3_move_monster_by(g, d, 0, (int)m);                         /* ... the middle band steps forwards */
					}
				}
			}
	}
	if (dg[MM3_DG_MOVE_COMBAT]) g->hooks->monsters_attack();
}

/* ---- combat order */
/* setSpeedTable: builds the turn order of a combat round.  The party members (by their speed stat) and the up to three monster
 * groups (by the monster speed column) are ranked fastest first into the 12 byte table at AD54h (slot numbers: 0..partySize-1
 * are the party in combat order, partySize.. the groups); ties keep the lower slot first.  The "current slot" (byte_28840) is
 * re-found in the new order so the character whose turn it was stays current. */
void mm3_set_speed_table(Mm3Game *g) {
	uint8_t *dg = g->dg;
	Mm3Rules rules = { dg };
	int8_t cur = (int8_t)dg[MM3_DG_CURRENT_SLOT];
	int had_current = dg[MM3_DG_CURRENT_SLOT] != 0xFF;
	int prev = (int8_t)dg[(uint16_t)(MM3_DG_SPEED_ORDER + cur)];
	int16_t speed[12];
	memset(dg + MM3_DG_SPEED_ORDER, 0xFF, 12);
	for (int i = 0; i < 12; i++) speed[i] = -1;
	int n = 0, fastest = 0;
	unsigned party = dg[MM3_DG_COMBAT_PARTY_SIZE];
	for (; (unsigned)n < party; n++) {
		const Mm3Character *ch = (const Mm3Character *)(dg + MM3_DG_PARTY_CHARS + dg[MM3_DG_COMBAT_ORDER + n] * sizeof(Mm3Character));
		speed[n] = (int16_t)mm3_char_stat(&rules, ch, MM3_STAT_SPEED, 0);
		if (speed[n] > fastest) fastest = speed[n];
	}
	const uint8_t *col = g->mem + (unsigned)rd16(g, MM3_DG_MON_SPEED + 2) * 16;
	int groups = 0;
	for (int r = 0; r < 3; r++) {
		unsigned m = dg[MM3_DG_MONSTER_ROWS + r];
		if (!m) continue;
		groups++;
		speed[n] = col[(uint16_t)(rd16(g, MM3_DG_MON_SPEED) + rd16(g, MM3_DG_MON_COLUMN_OFFSET + (m - 1) * 2))]; /* groups are numbered from 1 */
		if (speed[n] > fastest) fastest = speed[n];
		n++;
	}
	int out = 0;
	for (int s = fastest; s >= 0; s--)
		for (unsigned k = 0; k < party + 3; k++)
			if (speed[k] == s) dg[(uint16_t)(MM3_DG_SPEED_ORDER + out++)] = (uint8_t)k;
	if (had_current && (int8_t)dg[(uint16_t)(MM3_DG_SPEED_ORDER + (int8_t)dg[MM3_DG_CURRENT_SLOT])] != prev)
		for (int k = 0; k < (int)party + groups; k++)
			if ((int8_t)dg[MM3_DG_SPEED_ORDER + k] == prev) { dg[MM3_DG_CURRENT_SLOT] = (uint8_t)k; break; }
}

/* stopAttack(dx, dy): is the straight line from the party to the cell (dx, dy) away (one of them is 0) free of walls?
 * 0 when blocked.  Otherwise 1, or distance+1 when the party faces along that line (so callers can tell a shot ahead). */
int mm3_line_clear(const Mm3Game *g, int dx, int dy) {
	unsigned px = g->dg[MM3_DG_PARTY_X], py = g->dg[MM3_DG_PARTY_Y], facing = g->dg[MM3_DG_PARTY_FACING];
	#define WALL(x, y, m) mm3_maze_word(g, (int16_t)(x), (int16_t)(y), (m))
	if (dx > 0) {
		for (int s = 1; s <= dx; s++) if (WALL(px + s, py, 0x8)) return 0;
		return facing == 2 ? dx + 1 : 1;
	}
	if (dx < 0) {
		for (int s = dx; s != 0; s++) if (WALL(px + s, py, 0x800)) return 0;
		return facing == 3 ? -dx + 1 : 1;
	}
	if (dy > 0) {
		for (int s = 1; s <= dy; s++) if (WALL(px, py + s, 0x80)) return 0;
		return facing == 0 ? dy + 1 : 1;
	}
	for (int s = dy; s != 0; s++) if (WALL(px, py + s, 0x8000)) return 0;
	return facing == 1 ? -dy + 1 : 1;
	#undef WALL
}

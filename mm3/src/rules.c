#include "rules.h"

/* DGROUP offsets of the tables used here (docs/rules.md) */
enum {
	DG_STAT_VALUES = 0xF60,      /* 24 words */
	DG_STAT_BONUSES = 0xF90,     /* signed bytes */
	DG_AGE_ADJUST = 0xFD2,       /* 2 groups x 10 words */
	DG_AGE_RANGES = 0xFFA,       /* words */
	DG_ATTR_CATEGORY = 0xFBC,    /* upper bounds of the attribute enchantment ids per category */
	DG_ELEM_CATEGORY = 0xFCC,
	DG_ATTR_BONUS = 0xACD,       /* signed byte per attribute enchantment material */
	DG_ELEM_RESIST = 0xA27,
	DG_ARMOR_STRENGTH = 0xC87,   /* by item id */
	DG_METAL_AC = 0xA9F,         /* signed, by armour metal */
	DG_CLASS_BASE_HP = 0xE61,
	DG_RACE_HP = 0x5AB4,
	DG_RACE_SP = 0x5AA9,         /* signed bytes, 2 per race (intellect, personality) */
	DG_PARTY_YEAR = 0xEC36
};

static unsigned u8(const Mm3Rules *r, unsigned off) { return r->dg[off]; }
static int s8(const Mm3Rules *r, unsigned off) { return (int8_t)r->dg[off]; }
static unsigned u16(const Mm3Rules *r, unsigned off) { return (unsigned)(r->dg[off] | (r->dg[off + 1] << 8)); }
static int s16(const Mm3Rules *r, unsigned off) { return (int16_t)u16(r, off); }

int mm3_stat_bonus(const Mm3Rules *r, uint16_t value) {
	unsigned i = 0;
	while (u16(r, DG_STAT_VALUES + 2 * i) <= value) i++;
	return s8(r, DG_STAT_BONUSES + i);
}

int mm3_char_age(const Mm3Rules *r, const Mm3Character *ch, int ignore_temp) {
	int age = (int16_t)(s16(r, DG_PARTY_YEAR) - (int16_t)ch->birthYear);
	if (age > 0xFE) age = 0xFE;
	return (int16_t)(ignore_temp ? age : age + ch->tempAge);
}

static unsigned category(const Mm3Rules *r, unsigned table, unsigned id) {
	unsigned i = 0;
	while ((int)u8(r, table + i) < (int)id) i++;
	return i;
}

int mm3_item_scan(const Mm3Rules *r, const Mm3Character *ch, int which) {
	int sum = 0;
	for (int i = 0; i < 18; i++) {
		if (!ch->slotPresent[i] || (ch->slotFlags[i] & 0xC0)) continue; /* empty, cursed or broken */
		if (which < 0x0B && which != 3 && ch->slotAttribute[i]) { /* attribute enchantment: stat categories skip endurance's slot */
			int cat = (int)category(r, DG_ATTR_CATEGORY, ch->slotAttribute[i]);
			if (cat > 2) cat++;
			if (cat == which) sum += s8(r, DG_ATTR_BONUS + ch->slotAttribute[i]);
		}
		if (which > 0x0A && ch->slotElement[i]) { /* elemental resistance */
			int cat = (int)category(r, DG_ELEM_CATEGORY, ch->slotElement[i]) + 0x0B;
			if (cat == which) sum += u8(r, DG_ELEM_RESIST + ch->slotElement[i]);
		}
		if (which == 9) { /* armour class */
			sum += u8(r, DG_ARMOR_STRENGTH + ch->slotId[i]);
			if (ch->slotMetal[i]) {
				int plain = ch->slotId[i] != 0x2A && (ch->slotPresent[i] == 1 || ch->slotPresent[i] == 4 || ch->slotPresent[i] == 0x0D);
				if (!plain) sum += s8(r, DG_METAL_AC + ch->slotMetal[i]);
			}
		}
	}
	return (int16_t)sum;
}

/* Each condition (a count in conditions[]) changes the seven stats by +-1 per point: columns might, intellect, personality,
 * endurance, speed, accuracy, luck. */
static const int8_t CONDITION_EFFECT[8][7] = {
	/* 0 cursed        */ { 0, 0, 0, 0, 0, 0, -1 },
	/* 1 heart broken  */ { -1, -1, -1, -1, -1, -1, -1 },
	/* 2 weak          */ { -1, -1, -1, -1, -1, -1, -1 },
	/* 3 poisoned      */ { -1, 0, 0, 0, -1, -1, 0 },
	/* 4 diseased      */ { 0, -1, -1, -1, 0, 0, 0 },
	/* 5 insane        */ { 1, -1, -1, 0, 1, -1, 0 },
	/* 6 in love       */ { 1, 1, 1, 1, 1, 1, 1 },
	/* 7 drunk         */ { -1, -1, 1, -1, -1, -1, 1 }
};

int mm3_condition_mod(const Mm3Rules *r, const Mm3Character *ch, int which) {
	(void)r;
	int total[7] = { 0 };
	if (ch->conditions[0x0D] || ch->conditions[0x0E] || ch->conditions[0x0F]) return 0; /* dead, stoned, eradicated */
	for (int c = 0; c < 8; c++)
		for (int s = 0; s < 7; s++) total[s] += CONDITION_EFFECT[c][s] * ch->conditions[c];
	return (int16_t)total[which > 5 || which < 0 ? 6 : which];
}

int mm3_char_stat(const Mm3Rules *r, const Mm3Character *ch, int stat, int base_only) {
	int value = ch->stat[stat].permanent, temp = ch->stat[stat].temporary;
	if (stat != MM3_STAT_LUCK) { /* ageing: group 0 physical, group 1 mental */
		int group = (stat == MM3_STAT_INTELLECT || stat == MM3_STAT_PERSONALITY) ? 1 : 0;
		unsigned age = (uint16_t)mm3_char_age(r, ch, 0), i = 0;
		while (u16(r, DG_AGE_RANGES + 2 * i) <= age) i++;
		value += s16(r, DG_AGE_ADJUST + group * 20 + 2 * i);
	}
	value += mm3_item_scan(r, ch, stat);
	if (!base_only) value += mm3_condition_mod(r, ch, stat) + temp;
	value = (int16_t)value;
	return value < 1 ? 0 : value;
}

int mm3_char_level(const Mm3Character *ch) {
	int level = ch->level + ch->tempLevel;
	return level < 0 ? 0 : level;
}

/* 32-bit arithmetic as in the original, including its quirk: the equipment bonus is added as an unsigned word */
static uint16_t finish(int32_t value, int scan) {
	int32_t v = (int32_t)((uint32_t)value + (uint16_t)scan);
	return (uint16_t)(v < 0 ? 0 : v);
}

uint16_t mm3_max_hp(const Mm3Rules *r, const Mm3Character *ch) {
	int32_t hp = (int32_t)u8(r, DG_CLASS_BASE_HP + ch->charClass);
	hp += mm3_stat_bonus(r, (uint16_t)mm3_char_stat(r, ch, MM3_STAT_ENDURANCE, 0));
	hp += s8(r, DG_RACE_HP + ch->race);
	if (ch->skills[3]) hp += 1; /* bodybuilder */
	if (hp < 1) hp = 1;
	hp = (int32_t)((uint32_t)hp * (uint32_t)mm3_char_level(ch));
	return finish(hp, mm3_item_scan(r, ch, 7));
}

uint16_t mm3_max_sp(const Mm3Rules *r, const Mm3Character *ch) {
	if (!ch->hasSpells) return 0;
	int stat, skill;
	if (ch->charClass == 4 || ch->charClass == 2) { stat = MM3_STAT_INTELLECT; skill = 0x0D; } /* sorcerer, archer */
	else { stat = MM3_STAT_PERSONALITY; skill = 0x0C; }
	int hybrid = ch->charClass == 8 || ch->charClass == 9; /* druid, ranger: average of both stats */
	if (hybrid) skill = 2;
	int32_t first = 0, sp;
	for (int pass = 0;; pass++) {
		sp = mm3_stat_bonus(r, (uint16_t)mm3_char_stat(r, ch, stat, 0)) + 3;
		sp += s8(r, DG_RACE_SP + ch->race * 2 + stat);
		if (ch->skills[skill]) sp += 2;
		if (sp < 1) sp = 1;
		sp = (int32_t)((uint32_t)sp * (uint32_t)mm3_char_level(ch));
		if (ch->charClass != 4 && ch->charClass != 3 && ch->charClass != 8) sp >>= 1;
		if (pass == 0 && hybrid) { first = sp; stat = MM3_STAT_INTELLECT; continue; }
		break;
	}
	if (hybrid) sp = (int32_t)((uint32_t)sp + (uint32_t)first) >> 1;
	return finish(sp, mm3_item_scan(r, ch, 8));
}

int mm3_armor_class(const Mm3Rules *r, const Mm3Character *ch, int base_only) {
	int ac = mm3_stat_bonus(r, (uint16_t)mm3_char_stat(r, ch, MM3_STAT_SPEED, 0));
	ac += mm3_item_scan(r, ch, 9);
	if (!base_only) ac += ch->blessed + ch->acTemp;
	ac = (int16_t)ac;
	return ac < 1 ? 0 : ac;
}

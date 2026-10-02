#include "mm2_tavern.h"
#include "mm2_party.h"
#include "mm2_tables.h"

static int bracket_or_zero(int v) {
	int b = mm2_bracket(v);
	return (uint8_t)b >= 0xF2 ? 0 : b;
}

uint32_t mm2_tavern_specialty_price(int town, int idx) { return MM2_TAVERN_SPECIAL_PRICE[town * 3 + idx]; }
uint32_t mm2_tavern_drink_price(int kind) { return MM2_TAVERN_DRINK_PRICE[kind]; }

Mm2TavernResult mm2_tavern_feed(Mm2Roster *r, Mm2Char *payer, int town) {
	int i;
	if (!mm2_char_pay(payer, MM2_TAVERN_FOOD_PRICE[town])) return MM2_TAVERN_NO_GOLD;
	for (i = 0; i < mm2_party_size(r); i++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		if (c->raw[MC_FOOD] < 40) c->raw[MC_FOOD] = 40;
	}
	return MM2_TAVERN_OK;
}

/* TODO(review): drinks, ovl/2BRAIN.asm submenu at IDA ~0x1CB9C..0x1CD40 and the effect routine loc_1C94E (0x1C94E..0x1C9D4).
 * The first `limit` drinks of a kind per visit are plain (counter at DGROUP:5772, reset by loc_1D13C on entry); later ones add
 * the table bonus to a stat (0 Might, 1 Accuracy, 2 Personality, 3 Intellect, 4 LEVEL, 5 SPELL LEVEL: the pointer offsets
 * +6B/6F/6D/6C/71/72 and the tables DGROUP:4248 / 424E are read as written) and cost 2 Speed.  Strangely the bonus affects the
 * current level and spell level; that is what the code does but looks unintended or at least odd. */
Mm2TavernResult mm2_tavern_drink(Mm2Char *c, int kind, Mm2TavernVisit *visit, const Mm2Rng *rng) {
	static const int OFF[6] = {0x6B, 0x6F, 0x6D, 0x6C, 0x71, 0x72};
	if (mm2_c8(c, MC_CONDITION)) return MM2_TAVERN_DISABLED;
	if (!mm2_char_pay(c, MM2_TAVERN_DRINK_PRICE[kind])) return MM2_TAVERN_NO_GOLD;
	if (visit->drinks[kind] < (int)MM2_TAVERN_DRINK_LIMIT[kind]) {
		visit->drinks[kind]++;
	} else {
		uint8_t *stat = &c->raw[OFF[kind]];
		uint8_t n = (uint8_t)(*stat + MM2_TAVERN_DRINK_INC[kind]);
		if (n < MM2_TAVERN_DRINK_CAP[kind] && MM2_TAVERN_DRINK_INC[kind] <= n) *stat = n;
		if (c->raw[0x6E] >= 2) c->raw[0x6E] -= 2;
	}
	if (rng->range(rng->ud, 1, bracket_or_zero((int)mm2_c8(c, MC_ENDURANCE)) + 10) == 2) {
		mm2_char_reset_current_stats(c);
		c->raw[MC_CONDITION] |= 0x08;
		return MM2_TAVERN_SICK;
	}
	return MM2_TAVERN_OK;
}

/* TODO(review): specialties, ovl/2BRAIN.asm IDA ~0x1CD95..0x1CF5E and tavern_helper_a (0x1C9D8): price from the word table at
 * DGROUP:4208 (3 per town), flag word OR-ed into char +76h from DGROUP:04A4; 1 in (endurance bracket + 5) chance of a disease. */
Mm2TavernResult mm2_tavern_specialty(Mm2Char *c, int town, int idx, const Mm2Rng *rng) {
	uint16_t flag;
	if (mm2_c8(c, MC_CONDITION)) return MM2_TAVERN_DISABLED;
	if (!mm2_char_pay(c, MM2_TAVERN_SPECIAL_PRICE[town * 3 + idx])) return MM2_TAVERN_NO_GOLD;
	if (rng->range(rng->ud, 1, bracket_or_zero((int)mm2_c8(c, MC_ENDURANCE)) + 5) == 1) {
		c->raw[MC_CONDITION] |= 0x04;
		return MM2_TAVERN_SICK;
	}
	flag = MM2_TAVERN_SPECIAL_FLAG[town * 3 + idx];
	c->raw[0x76] |= (uint8_t)flag;
	c->raw[0x77] |= (uint8_t)(flag >> 8);
	return MM2_TAVERN_OK;
}

int mm2_tavern_rumour_index(int day) {
	if (day == 0xB4) return 3;
	if (day % 30 == 0) return day / 30;
	return (day & 1) ^ 1;
}

/* TODO(review): tip, ovl/2BRAIN.asm IDA 0x1CF84..0x1D032: costs 1 gold, then a 1 in (endurance bracket + 5) chance to hear the
 * rumour sub_1CA46 selects (mm2_tavern_rumour_index); the message table at DGROUP:5676 + town*16 is not decoded here. */
Mm2TavernResult mm2_tavern_tip(Mm2Char *c, int *heard, int *rumour, int dayOfYear, const Mm2Rng *rng) {
	*heard = 0;
	if (mm2_c8(c, MC_CONDITION)) return MM2_TAVERN_DISABLED;
	if (!mm2_char_pay(c, 1)) return MM2_TAVERN_NO_GOLD;
	if (rng->range(rng->ud, 1, bracket_or_zero((int)mm2_c8(c, MC_ENDURANCE)) + 5) == 1) {
		*heard = 1;
		*rumour = mm2_tavern_rumour_index(dayOfYear);
	}
	return MM2_TAVERN_OK;
}

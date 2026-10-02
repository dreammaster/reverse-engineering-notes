#include "mm2_town.h"
#include "mm2_spells.h"
#include "mm2_tables.h"

#define MM2_SPELL_LEVEL_OF(s) MM2_SPELL_LEVEL[s]

uint32_t mm2_price_decode(int code) {
	uint32_t v = (uint32_t)(code & 0x1F);
	if (code & 0x20) v *= 10;
	if (code & 0x40) v *= 100;
	if (code & 0x80) v *= 1000;
	return v;
}

/* TODO(review): costs from docs/shops.md (read from ovl/2TEMPLE.asm temple_menu, IDA 0x1CA88).  I did not re-read the code
 * for this port: the base amounts (10/100/1000), the use of the current level (+71h; could be the base level +20h) and the
 * town multiplier DGROUP:46A8 are taken from my earlier notes. */
uint32_t mm2_temple_restore_cost(const Mm2Char *c, int town) {
	unsigned cond = mm2_c8(c, MC_CONDITION);
	uint32_t base;
	if (cond == 0xFF)
		base = 1000;
	else if (cond >= 0x80)
		base = 100;
	else if (cond != 0 || mm2_c16(c, MC_HP) < mm2_c16(c, MC_HP_MAX))
		base = 10;
	else
		return 0;
	return base * mm2_c8(c, MC_LEVEL) * MM2_TEMPLE_MULT[town];
}

uint32_t mm2_temple_alignment_cost(const Mm2Char *c, int town) {
	if (mm2_c8(c, MC_ALIGN) == mm2_c8(c, MC_ORIG_ALIGN)) return 0;
	return 100u * mm2_c8(c, MC_LEVEL) * MM2_TEMPLE_MULT[town];
}

uint32_t mm2_temple_donation_cost(int town) {
	return 100u * MM2_TEMPLE_MULT[town];
}

static int stock(const uint8_t *spell, const uint8_t *price, int town, int spellOut[4], uint32_t priceOut[4]) {
	int i, n = 0;
	for (i = 0; i < 4; i++) {
		if (spell[town * 4 + i] == 128) continue;
		spellOut[n] = spell[town * 4 + i];
		priceOut[n] = mm2_price_decode(price[town * 4 + i]);
		n++;
	}
	return n;
}

int mm2_temple_stock(int town, int spell[4], uint32_t price[4]) {
	return stock(MM2_TEMPLE_SPELL, MM2_TEMPLE_PRICE, town, spell, price);
}

int mm2_guild_stock(int town, int spell[4], uint32_t price[4]) {
	return stock(MM2_GUILD_SPELL, MM2_GUILD_PRICE, town, spell, price);
}

int mm2_char_pay(Mm2Char *c, uint32_t amount) {
	uint32_t gold = mm2_c32(c, MC_GOLD);
	int i;
	if (gold < amount) return 0;
	gold -= amount;
	for (i = 0; i < 4; i++)
		c->raw[MC_GOLD + i] = (uint8_t)(gold >> (8 * i));
	return 1;
}

Mm2ShopResult mm2_temple_restore(Mm2Char *c, int town) {
	uint32_t cost = mm2_temple_restore_cost(c, town);
	if (!cost) return MM2_SHOP_NOTHING_TO_DO;
	if (!mm2_char_pay(c, cost)) return MM2_SHOP_NO_GOLD;
	c->raw[MC_CONDITION] = 0;
	c->raw[MC_HP] = c->raw[MC_HP_MAX];
	c->raw[MC_HP + 1] = c->raw[MC_HP_MAX + 1];
	return MM2_SHOP_OK;
}

Mm2ShopResult mm2_temple_restore_alignment(Mm2Char *c, int town) {
	uint32_t cost = mm2_temple_alignment_cost(c, town);
	if (!cost) return MM2_SHOP_NOTHING_TO_DO;
	if (!mm2_char_pay(c, cost)) return MM2_SHOP_NO_GOLD;
	c->raw[MC_ALIGN] = c->raw[MC_ORIG_ALIGN];
	return MM2_SHOP_OK;
}

/* TODO(review): donation effects from docs/shops.md (ovl/2TEMPLE.asm temple_menu, IDA 0x1CA88): 90 % chance of blessings with
 * Light 200, Magic 60, Forces 60, Levitate / Walk on Water / Guard Dog 1 in the party effect bytes (DGROUP:03D5..03DA = game
 * bytes 1DC25..1DC2A, see docs/save-format.md).  The donation counter word_23130, the per-town bit of byte_1DC32 (DGROUP:470C)
 * and the special event when all five temples were visited are NOT ported, and "other effect bytes set" in the docs was not
 * itemised. */
Mm2ShopResult mm2_temple_donate(Mm2Char *payer, Mm2State *state, int town, int *blessed, const Mm2Rng *rng) {
	static const struct { unsigned dg; uint8_t v; } FX[] = {{0x3D5, 200}, {0x3D6, 60}, {0x3D7, 60}, {0x3D8, 1}, {0x3D9, 1}, {0x3DA, 1}};
	unsigned i;
	*blessed = 0;
	if (!mm2_char_pay(payer, mm2_temple_donation_cost(town))) return MM2_SHOP_NO_GOLD;
	if (rng->range(rng->ud, 1, 100) <= 90) {
		*blessed = 1;
		for (i = 0; i < sizeof(FX) / sizeof(FX[0]); i++) {
			uint8_t *p = mm2_state_ptr(state, FX[i].dg);
			if (p) *p = FX[i].v;
		}
	}
	return MM2_SHOP_OK;
}

/* TODO(review): the rules (spell level <= the character's spell level, spell not known yet, class list) come from
 * docs/shops.md ("362C" known-spells check in ovl/2TEMPLE.asm, IDA ~0x1CB9C..0x1CF7x) and were not re-read in the code; the
 * class lists (Paladin/Cleric = cleric list, Archer/Sorcerer = sorcerer list) follow docs/spells.md. */
Mm2ShopResult mm2_buy_spell(Mm2Char *c, int spell, uint32_t price) {
	int cls = (int)mm2_c8(c, MC_CLASS), cleric = spell >= 48, bit = cleric ? spell - 48 : spell;
	int allowed = cleric ? (cls == MM2_PALADIN || cls == MM2_CLERIC) : (cls == MM2_ARCHER || cls == MM2_SORCERER);
	if (!allowed) return MM2_SHOP_WRONG_CLASS;
	if ((int)mm2_c8(c, MC_SPELL_LEVEL) < MM2_SPELL_LEVEL_OF(spell)) return MM2_SHOP_LEVEL_TOO_LOW;
	if (c->raw[MC_SPELL_BITS + (bit >> 3)] & (1 << (bit & 7))) return MM2_SHOP_KNOWN;
	if (!mm2_char_pay(c, price)) return MM2_SHOP_NO_GOLD;
	c->raw[MC_SPELL_BITS + (bit >> 3)] |= (uint8_t)(1 << (bit & 7));
	return MM2_SHOP_OK;
}

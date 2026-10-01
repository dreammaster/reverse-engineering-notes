#include "mm2_town.h"
#include "mm2_tables.h"

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

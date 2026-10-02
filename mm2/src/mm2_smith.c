#include "mm2_smith.h"
#include "mm2_tables.h"
#include "mm2_town.h"

/* TODO(review): ovl/2SMITH.asm sub_1C8E0 / smith_load_stock (IDA 0x1C8E0..0x1CA52).  The category-to-table mapping (1 = 43C8,
 * 2 = 447C with day bonus, 3 = 4404, 4 = 4440 with charges) was read from the code; the day bonus uses the day-of-year of the
 * current era (DGROUP:03A2[era]) which the caller must supply.  Side effects for map 1 / category 4 (byte_23060/23062,
 * 0x1CA3F) are not ported. */
void mm2_smith_stock(int town, int category, int dayOfYear, Mm2SmithSlot out[6]) {
	int i, o = town * 6;
	for (i = 0; i < 6; i++) {
		out[i].bonus = 0;
		out[i].charges = 0;
		switch (category) {
		case 1:
			out[i].item = MM2_SMITH_A_ID[o + i];
			out[i].bonus = MM2_SMITH_A_BONUS[o + i];
			break;
		case 2: {
			int r = dayOfYear % 30, q = dayOfYear / 30;
			out[i].item = MM2_SMITH_B_ID[o + i];
			out[i].bonus = r == 29 ? MM2_SMITH_DAY_BONUS_SPECIAL[q < 6 ? q : 5] : MM2_SMITH_DAY_BONUS[r];
			break;
		}
		case 3:
			out[i].item = MM2_SMITH_C_ID[o + i];
			out[i].bonus = MM2_SMITH_C_BONUS[o + i];
			break;
		default:
			out[i].item = MM2_SMITH_D_ID[o + i];
			out[i].charges = MM2_SMITH_D_CHARGES[o + i];
			break;
		}
	}
}

uint32_t mm2_smith_price(const Mm2Item *item, int bonus, Mm2SmithMode mode, int merchant) {
	uint32_t p;
	bonus &= 0x3F;
	if (mode == MM2_SMITH_IDENTIFY)
		return bonus ? 100u * (uint32_t)bonus : 10u;
	p = item->price;
	if (bonus) {
		p *= 2;
		bonus--;
	}
	p += 1000u * (uint32_t)bonus;
	if (mode == MM2_SMITH_SELL) {
		p >>= 1;
		if (!merchant) p >>= 1;
	} else if (merchant) {
		p >>= 1;
	}
	return p;
}

/* TODO(review): buy/sell flow from ovl/2SMITH.asm sub_1C776 (buy, IDA 0x1C776..0x1C7F6) and loc_1C5A2 (sell, 0x1C5A2..0x1C5F4):
 * an incapacitated character (+26h != 0) cannot trade (message 8), an empty slot gives message 6, no free backpack slot message
 * 2, not enough gold message 4.  Selling credits the price through smith_credit_gold (0x1C130) which I assumed adds to the
 * character's own gold. */
Mm2SmithResult mm2_smith_buy(Mm2Char *c, const Mm2SmithSlot *slot, const Mm2Item *items, int merchant) {
	int i;
	if (mm2_c8(c, MC_CONDITION)) return MM2_SMITH_DISABLED;
	if (!slot->item) return MM2_SMITH_NO_ITEM;
	for (i = 0; i < 6 && c->raw[MC_PACK_ID + i]; i++) {}
	if (i == 6) return MM2_SMITH_PACK_FULL;
	if (!mm2_char_pay(c, mm2_smith_price(&items[slot->item], slot->bonus, MM2_SMITH_BUY_A, merchant))) return MM2_SMITH_NO_GOLD;
	c->raw[MC_PACK_ID + i] = slot->item;
	c->raw[0x40 + i] = slot->charges;
	c->raw[MC_PACK_FLAGS + i] = slot->bonus;
	return MM2_SMITH_DONE;
}

Mm2SmithResult mm2_smith_sell(Mm2Char *c, int packSlot, const Mm2Item *items, int merchant) {
	uint32_t gold, price;
	int id = (int)mm2_c8(c, MC_PACK_ID + packSlot), j;
	if (mm2_c8(c, MC_CONDITION)) return MM2_SMITH_DISABLED;
	if (!id) return MM2_SMITH_NO_ITEM;
	price = mm2_smith_price(&items[id], (int)(mm2_c8(c, MC_PACK_FLAGS + packSlot) & 0x3F), MM2_SMITH_SELL, merchant);
	gold = mm2_c32(c, MC_GOLD) + price;
	for (j = 0; j < 4; j++)
		c->raw[MC_GOLD + j] = (uint8_t)(gold >> (8 * j));
	for (j = packSlot; j < 5; j++) {   /* close the gap like char_backpack_remove */
		c->raw[MC_PACK_ID + j] = c->raw[MC_PACK_ID + j + 1];
		c->raw[0x40 + j] = c->raw[0x40 + j + 1];
		c->raw[MC_PACK_FLAGS + j] = c->raw[MC_PACK_FLAGS + j + 1];
	}
	c->raw[MC_PACK_ID + 5] = c->raw[0x40 + 5] = c->raw[MC_PACK_FLAGS + 5] = 0;
	return MM2_SMITH_DONE;
}

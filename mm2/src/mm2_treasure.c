#include "mm2_treasure.h"

#include <string.h>

/* TODO(review): port of ovl/2MISC.asm treasure_share (IDA 0x1C64A..0x1C7A8): gold / number of non-hireling members (ids < 18h),
 * gems / party size, every member gets the gem share (+5Ch) but only non-hirelings the gold (+66h); remainders are lost.  Items go
 * to the first member (any, hirelings included) with a free backpack slot (treasure_give_item 0x1C538).  In the original the
 * search screen decides *when* this happens; the order/timing after a fight is not ported. */
Mm2ShareResult mm2_treasure_share(Mm2Roster *r, Mm2Treasure *t) {
	Mm2ShareResult res;
	int i, k, chars = 0, n = mm2_party_size(r);
	memset(&res, 0, sizeof(res));
	for (i = 0; i < 3; i++) res.foundBy[i] = -1;
	for (i = 0; i < n; i++)
		if (mm2_party_member(r, i) < MM2_FIRST_HIRELING) chars++;
	res.goldShare = chars ? t->gold / (uint32_t)chars : 0;
	res.gemShare = n ? t->gems / n : 0;
	for (i = 0; i < n; i++) {
		Mm2Char *c = &r->chars[mm2_party_member(r, i)];
		unsigned gems = mm2_c16(c, MC_GEMS) + (unsigned)res.gemShare;
		c->raw[MC_GEMS] = (uint8_t)gems;
		c->raw[MC_GEMS + 1] = (uint8_t)(gems >> 8);
		if (mm2_party_member(r, i) < MM2_FIRST_HIRELING) {
			uint32_t g = mm2_c32(c, MC_GOLD) + res.goldShare;
			for (k = 0; k < 4; k++) c->raw[MC_GOLD + k] = (uint8_t)(g >> (8 * k));
		}
	}
	for (k = 0; k < 3; k++) {
		int placed = 0;
		if (!t->items[k].item) continue;
		for (i = 0; i < n && !placed; i++) {
			Mm2Char *c = &r->chars[mm2_party_member(r, i)];
			int s;
			for (s = 0; s < 6; s++)
				if (!c->raw[MC_PACK_ID + s]) {
					c->raw[MC_PACK_ID + s] = t->items[k].item;
					c->raw[0x40 + s] = t->items[k].charges;
					c->raw[MC_PACK_FLAGS + s] = t->items[k].flags;
					res.foundBy[k] = i;
					placed = 1;
					break;
				}
		}
		if (placed) memset(&t->items[k], 0, sizeof(t->items[k]));
		else res.backpacksFull = 1;
	}
	t->gold = 0;
	t->gems = 0;
	return res;
}

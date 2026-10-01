#include "mm2_smith.h"
#include "mm2_tables.h"

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

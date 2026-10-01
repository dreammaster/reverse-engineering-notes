#include "mm2_reward.h"
#include "mm2_tables.h"

static int rnd(const Mm2Rng *r, int lo, int hi) {
	return r->range(r->ud, lo, hi);
}

void mm2_monster_reward(Mm2Loot *loot, const Mm2Monster *m, int id, const Mm2Rng *rng) {
	if (m->dropsGems) loot->gems += rnd(rng, 1, 10);
	if (m->goldClass) {
		uint8_t v = (uint8_t)id;
		int base;
		if (m->goldClass == 2) v >>= 4;
		if (m->goldClass >= 3) v >>= 1;
		v = (uint8_t)(v + rnd(rng, 1, v));
		base = rnd(rng, 1, 50) + 6;
		if (m->goldClass == 1) v = 0;
		loot->gold += (uint32_t)base + ((uint32_t)v << 8);
	}
	if (m->itemClass && m->itemClass >= loot->itemClass) {
		loot->itemTier = id >> 4;
		loot->itemClass = m->itemClass;
	}
	loot->exp += (uint32_t)m->exp;
}

static void drop(const Mm2Loot *loot, int quality, const Mm2Item *items, Mm2TreasureItem *out, const Mm2Rng *rng) {
	int roll, row = 0, id;
	uint8_t flags = 0, charges = 0;
	if (quality > 2) quality = 2;
	roll = rnd(rng, 1, 100);
	while (row < 7 && MM2_TREASURE_THRESHOLD[row] < roll)
		row++;
	id = MM2_TREASURE_ROWS[row * 4] + rnd(rng, 1, MM2_TREASURE_ROWS[row * 4 + 1 + quality]);
	if (items[id].useEffect) charges = MM2_TREASURE_CHARGES[quality];
	if (items[id].bonus != 0xF0 && loot->itemTier >= 2) {
		int b = rnd(rng, 1, 7);
		if (loot->itemTier >= 2 && loot->itemTier <= 12) b = rnd(rng, 1, loot->itemTier);
		if (loot->itemTier == 13) b = rnd(rng, 1, 21) + 11;
		if (b >= 5) {
			int r = rnd(rng, 1, 100);
			flags = (uint8_t)b;
			if (r < 41) flags |= 0x80;
			else if (r < 71) flags |= 0x40;
			else flags |= 0xC0;
		} else {
			flags = (uint8_t)b;
		}
	}
	out->item = (uint8_t)id;
	out->flags = flags;
	out->charges = charges;
}

int mm2_treasure_roll(const Mm2Loot *loot, const Mm2Item *items, Mm2TreasureItem out[3], const Mm2Rng *rng) {
	int roll = rnd(rng, 1, 100), n, i, quality = loot->itemClass;
	n = roll < 11 ? 3 : roll < 46 ? 2 : roll < 91 ? 1 : 0;
	for (i = 0; i < n; i++) {
		if (quality > 0) quality--;
		drop(loot, quality, items, &out[i], rng);
	}
	return n;
}

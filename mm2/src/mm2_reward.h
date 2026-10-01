/* Battle rewards (2COMBAT combat_monster_rewards 188FC, combat_treasure_roll 19B88,
 * combat_drop_treasure_item 19A3C; docs/combat.md). */
#ifndef MM2_REWARD_H
#define MM2_REWARD_H

#include "mm2_combat.h"

typedef struct {
	uint32_t gold;
	int gems;
	uint32_t exp;        /* split among the survivors by the caller */
	int itemClass;       /* best treasure class so far (byte_22CE4) */
	int itemTier;        /* monster id >> 4 of the monster that set it (byte_22CE7) */
} Mm2Loot;

/* Adds the loot of one killed monster (record `m`, id `id`) to `loot`. */
void mm2_monster_reward(Mm2Loot *loot, const Mm2Monster *m, int id, const Mm2Rng *rng);

typedef struct {
	uint8_t item;      /* item id */
	uint8_t flags;     /* top bits: alignment restriction; low bits: bonus */
	uint8_t charges;
} Mm2TreasureItem;

/* Rolls the victory treasure; returns the number of items (0-3) written to out[3]. */
int mm2_treasure_roll(const Mm2Loot *loot, const Mm2Item *items, Mm2TreasureItem out[3], const Mm2Rng *rng);

#endif

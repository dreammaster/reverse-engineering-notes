/* Combat formulas (docs/combat.md), ported from 2COMBAT: combat_party_attack (18DAA) and
 * combat_monster_melee (18398).  Pure functions driven by an injectable random source. */
#ifndef MM2_COMBAT_H
#define MM2_COMBAT_H

#include "mm2_data.h"

typedef struct {
	int (*range)(void *ud, int lo, int hi);   /* the game's rand_range(lo, hi), inclusive */
	void *ud;
} Mm2Rng;

/* Party-wide effect bytes that feed the attack (DGROUP:03E0.., saved in the state block). */
typedef struct {
	uint8_t accuracyBonus;   /* 1DC33 */
	uint8_t damageBonus;     /* 1DC37, added once if anything hit */
	uint8_t hitFloor;        /* 1DC2B: an attack roll below this value misses */
} Mm2AttackMods;

typedef enum { MM2_HIT_NORMAL, MM2_HIT_BACKSTAB, MM2_HIT_CRITICAL } Mm2HitKind;

typedef struct {
	int swings, hits, damage;
	Mm2HitKind kind;
} Mm2AttackResult;

/* One character's attack action against a monster of armour class `monsterAc`.  `shooting` selects the
 * missile weapon fields. */
Mm2AttackResult mm2_party_attack(const Mm2Char *c, int monsterAc, int shooting, const Mm2AttackMods *mods, const Mm2Rng *rng);

typedef struct {
	int blows, hits, damage;
} Mm2MonsterAttackResult;

/* Monster melee blows against a character with armour class `charAc`.  tier = monster id >> 4. */
Mm2MonsterAttackResult mm2_monster_melee(const Mm2Monster *m, int tier, int charAc, int frightened, int weakened, const Mm2Rng *rng);

/* Percentage chance for a monster blow to hit (before the frightened halving). */
int mm2_monster_hit_chance(int tier, int charAc);

/* Spell damage helper sub_1A82C: sum over `level` iterations of rand(1, dice) + bonus (dice 0: bonus only). */
int mm2_spell_damage_roll(int level, int dice, int bonus, const Mm2Rng *rng);

#endif

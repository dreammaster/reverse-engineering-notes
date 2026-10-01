#include "mm2_combat.h"
#include "mm2_tables.h"

enum { C_WEAPON_DICE = 0x4C, C_WEAPON_BONUS = 0x4D, C_MISSILE_DICE = 0x4E, C_MISSILE_BONUS = 0x4F, C_CRIT_STAT = 0x72 };

static int rnd(const Mm2Rng *r, int lo, int hi) {
	return r->range(r->ud, lo, hi);
}

Mm2AttackResult mm2_party_attack(const Mm2Char *c, int monsterAc, int shooting, const Mm2AttackMods *mods, const Mm2Rng *rng) {
	Mm2AttackResult res = {0, 0, 0, MM2_HIT_NORMAL};
	int cls = (int)mm2_c8(c, MC_CLASS), level = (int)mm2_c8(c, MC_LEVEL);
	int cursed = (int)(mm2_c8(c, MC_CONDITION) & 1);
	int swings = level / MM2_SWING_DIV[cls] + 1;
	int hitBase = level / MM2_HIT_DIV[cls];
	uint8_t dice = (uint8_t)mm2_c8(c, C_WEAPON_DICE);
	uint8_t weaponBonus = (uint8_t)mm2_c8(c, shooting ? C_MISSILE_BONUS : C_WEAPON_BONUS);
	uint8_t dmgBonus = 0;
	uint8_t hitBonus;
	int i, anyHit = 0;

	if (shooting) {
		if (cls != MM2_ARCHER)
			dice = (uint8_t)mm2_c8(c, C_MISSILE_DICE);
		else
			dmgBonus = (uint8_t)rnd(rng, 1, level > 100 ? 100 : level);
	}
	dmgBonus = (uint8_t)(dmgBonus + weaponBonus);
	dmgBonus = (uint8_t)(dmgBonus + mm2_bracket((int)mm2_c8(c, MC_CUR_STATS)));         /* Might */
	hitBonus = (uint8_t)(weaponBonus + mm2_bracket((int)mm2_c8(c, MC_CUR_STATS + 4)));   /* Accuracy */
	hitBonus = (uint8_t)(hitBonus + mods->accuracyBonus);

	res.swings = swings;
	for (i = 0; i < swings; i++) {
		int r = rnd(rng, 1, 100), hit = 0;
		if (r < 6) {
			hit = 1;
		} else if (r >= 9) {
			int top = (cursed ? 3 : 25) + hitBase, n;
			if (top > 250) top = 250;
			n = rnd(rng, 1, top) + hitBonus;
			if (n > 255) {
				hit = 1;
			} else if (n > 10) {
				int diff = n - mods->hitFloor;
				if (diff >= 0 && diff < 0x80 && n >= monsterAc) hit = 1;
			}
		}
		if (hit) {
			int d = rnd(rng, 1, dice) + dmgBonus;
			if (d > 250) d = 1;
			res.damage += d;
			res.hits++;
			anyHit = 1;
		}
	}
	if (anyHit) res.damage += mods->damageBonus;
	if (!shooting && (cls == MM2_ROBBER || cls == MM2_NINJA)) {
		int stat = (int)mm2_c8(c, C_CRIT_STAT);
		int r = rnd(rng, 1, 100 + (stat > 100 ? 100 : stat));
		if (cls == MM2_ROBBER && (r > 90 || r < 5)) {
			res.kind = MM2_HIT_BACKSTAB;
			res.damage *= 2;
		} else if (cls == MM2_NINJA && (r > 94 || r < 5)) {
			res.kind = MM2_HIT_CRITICAL;
			res.damage *= 4;
		}
	}
	return res;
}

int mm2_monster_hit_chance(int tier, int charAc) {
	int t = MM2_MONSTER_TOHIT[tier & 15];
	return charAc > t ? 5 : t - charAc;
}

Mm2MonsterAttackResult mm2_monster_melee(const Mm2Monster *m, int tier, int charAc, int frightened, int weakened, const Mm2Rng *rng) {
	Mm2MonsterAttackResult res = {m->blows, 0, 0};
	int p = mm2_monster_hit_chance(tier, charAc), i;
	if (frightened) p >>= 1;
	for (i = 0; i < m->blows; i++) {
		int roll = rnd(rng, 10, 1009) / 10;   /* the game rolls 10..1009 and divides by 10: d100 */
		if (p >= roll) {
			res.hits++;
			res.damage += rnd(rng, 1, m->damageDie);
		}
	}
	if (weakened) res.damage >>= 1;
	return res;
}

int mm2_spell_damage_roll(int level, int dice, int bonus, const Mm2Rng *rng) {
	int total = 0, i;
	for (i = 0; i < level; i++)
		total += (dice ? rnd(rng, 1, dice) : 0) + bonus;
	return total;
}

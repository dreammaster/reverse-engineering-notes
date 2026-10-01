/* Spell data and costs (docs/spells.md).  Indices 0-47 sorcerer list, 48-95 cleric list. */
#ifndef MM2_SPELLS_H
#define MM2_SPELLS_H

#include "mm2_combat.h"

extern const char *const MM2_SPELL_NAMES[96];
extern const uint8_t MM2_SPELL_LEVEL[96];   /* 1-9 */

/* spell_calc_cost (13AEA): spell point cost for a caster of `level` against `monsterCount` monsters. */
int mm2_spell_sp_cost(const Mm2Spell *table, int spell, int level, int monsterCount);

/* Combat spell parameters (docs/spells.md, read from 2CAST2). */
typedef struct {
	int targets;     /* monsters hit; 10 = all */
	int dice;        /* per caster level: rand(1, dice) + bonus; dice < 0: fixed damage */
	int bonus;
	int fixedDamage; /* used when perLevel == 0 */
	int perLevel;    /* 1: damage = sum over caster level; 0: fixed (one roll) */
	int element;     /* 1 fire, 2 electricity, 3 cold, 4 acid, 0 none */
} Mm2CombatSpell;

/* Returns 1 and fills out for the damage spells whose parameters are known; 0 for other spells. */
int mm2_combat_spell(int spell, Mm2CombatSpell *out);

/* Healing spells: HP healed by First Aid / Cure Wounds (fixed) -- Power Cure rolls level x 1d10. */
int mm2_heal_amount(int spell, int level, const Mm2Rng *rng);
/* Applies healing like sub_1CE46: fails for dead (>= 80h); clears asleep/unconscious; caps at max HP. Returns 1 on success. */
int mm2_heal_character(Mm2Char *c, int amount);

/* Damage of one cast by a caster of `level`. */
int mm2_combat_spell_damage(const Mm2CombatSpell *s, int level, const Mm2Rng *rng);

#endif

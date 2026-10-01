#include "mm2_spells.h"

/* TODO(review): resident mm2.asm spell_calc_cost (IDA 0x13AEA..0x13B64).  The original tests bit 6 of the spell's second
 * byte (40h) to add 10 instead of 8; Mm2Spell does not carry that bit, so I special-cased spell 46 (Star Burst) and used
 * "full monster count" for spells 42 and 46 (it compares the table address against 7DB4h/7DBCh).  Verify against the
 * data bytes of SPELLS.DAT. */
int mm2_spell_sp_cost(const Mm2Spell *table, int spell, int level, int monsterCount) {
	const Mm2Spell *sp = &table[spell];
	int mons = monsterCount > 10 ? 10 : monsterCount;
	if (spell == 42 || spell == 46) mons = monsterCount;   /* Meteor Shower and Star Burst use the full count */
	if (sp->spCost) return sp->spCost;
	if (sp->perLevel == 0) {
		/* special formula: 8 (10 for the strongest) + one per monster; flag 40h of the byte is bit 6 of locRestrict's byte */
		return (spell == 46 ? 10 : 8) + mons;
	}
	return level * sp->perLevel;
}

static const struct {
	int spell;
	Mm2CombatSpell p;
} COMBAT[] = {
	/*        targets dice bonus fixed perLevel element */
	{2, {1, 5, 1, 0, 1, 0}},    /* Energy Blast */
	{3, {1, 5, 3, 0, 0, 1}},    /* Flame Arrow */
	{8, {1, 9, 7, 0, 0, 2}},    /* Electric Arrow */
	{17, {4, 5, 1, 0, 1, 2}},   /* Lightning Bolt */
	{20, {1, 0, 6, 0, 1, 3}},   /* Cold Beam */
	{22, {6, 5, 1, 0, 1, 1}},   /* Fire Ball */
	{26, {1, 0, 0, 100, 0, 0}}, /* Disrupt */
	{28, {10, 7, 1, 0, 1, 0}},  /* Sand Storm */
	{33, {3, 0, 10, 0, 1, 3}},  /* Fantastic Freeze */
	{35, {1, 0, 20, 0, 1, 2}},  /* Super Shock */
	{36, {10, 11, 1, 0, 1, 0}}, /* Dancing Sword */
	{40, {1, 21, 19, 0, 1, 1}}, /* Incinerate */
	{41, {10, 9, 7, 0, 1, 2}},  /* Mega Volts */
	{42, {10, 21, 24, 0, 0, 0}}, /* Meteor Shower */
	{44, {1, 0, 0, 1000, 0, 0}}, /* Implosion */
	{45, {10, 16, 4, 0, 1, 1}}, /* Inferno */
	{46, {10, 161, 39, 0, 0, 0}}, /* Star Burst */
	{58, {1, 12, 3, 0, 0, 0}},  /* Pain */
	{62, {5, 0, 0, 25, 0, 3}},  /* Cold Ray */
};

/* TODO(review): values transcribed from the 2CAST2 handlers (ovl/2CAST2.asm, jump table in cast_spell_dispatch IDA 0x1CF2C,
 * handlers 0x1C130..0x1D100) by a condensed read of the push sequences to combat_party_spell_hits (ovl/2COMBAT.asm 0x18696)
 * and sub_1A82C (0x1A82C).  Argument order (targets, element) and which handler belongs to which spell were inferred;
 * Acid Stream/Acid Spray and several status spells were not decoded.  The element numbers are guesses beyond 1-4. */
int mm2_combat_spell(int spell, Mm2CombatSpell *out) {
	unsigned i;
	for (i = 0; i < sizeof(COMBAT) / sizeof(COMBAT[0]); i++)
		if (COMBAT[i].spell == spell) {
			*out = COMBAT[i].p;
			return 1;
		}
	return 0;
}

int mm2_combat_spell_damage(const Mm2CombatSpell *s, int level, const Mm2Rng *rng) {
	if (s->perLevel)
		return mm2_spell_damage_roll(level, s->dice, s->bonus, rng);
	if (s->dice) return rng->range(rng->ud, 1, s->dice) + s->bonus;
	return s->fixedDamage;
}

int mm2_heal_amount(int spell, int level, const Mm2Rng *rng) {
	if (spell == 51) return 8;     /* First Aid */
	if (spell == 55) return 15;    /* Cure Wounds */
	if (spell == 53) {             /* Power Cure: level x 1d10 */
		int i, t = 0;
		for (i = 0; i < level; i++)
			t += rng->range(rng->ud, 1, 10);
		return t;
	}
	return 0;
}

int mm2_heal_character(Mm2Char *c, int amount) {
	unsigned cond = c->raw[MC_CONDITION], hp, max;
	if (cond >= 0x80) return 0;
	c->raw[MC_CONDITION] = (uint8_t)(cond & 0x2F);
	hp = mm2_c16(c, MC_HP) + (unsigned)amount;
	max = mm2_c16(c, MC_HP_MAX);
	if (hp > max) hp = max;
	c->raw[MC_HP] = (uint8_t)hp;
	c->raw[MC_HP + 1] = (uint8_t)(hp >> 8);
	return 1;
}

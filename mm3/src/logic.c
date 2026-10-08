#include "logic.h"

#include <string.h>

static unsigned rd16(const Mm3Game *g, unsigned off) { return g->dg[off] | (g->dg[off + 1] << 8); }

Mm3Character *mm3_party_member(const Mm3Game *g, unsigned index) {
	return (Mm3Character *)(g->dg + MM3_DG_PARTY_CHARS + index * sizeof(Mm3Character));
}

/* Experience needed for level `level`: levels 2..11 grow by a class-dependent base doubling each level (base << (level - 2)), from level 12 on
 * the requirement grows by a fixed 0xFA000 (1,024,000) per level on top of the level-12 value. */
static uint32_t level_threshold(const Mm3Game *g, const Mm3Character *ch, unsigned level) {
	unsigned linear, shift;
	if (level < 12) { linear = 0; shift = level - 1; }
	else { linear = level - 11; shift = 10; }
	int32_t base = (int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass);
	return (uint32_t)((int32_t)linear * 0xFA000 + (base << shift));
}

uint32_t mm3_experience_total(const Mm3Game *g, const Mm3Character *ch) {
	if (ch->level - 1 == 0) return ch->experience;                       /* level 1: just the stored experience */
	unsigned level = ch->level - 1; /* the original works with level - 1 here */
	uint32_t base;
	if (level < 12) base = (uint32_t)((int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass) << (level - 1));
	else base = (uint32_t)((int32_t)(level - 11) * 0xFA000 + ((int16_t)rd16(g, MM3_DG_CLASS_XP + 2 * ch->charClass) << 10));
	return base + ch->experience;
}

uint32_t mm3_experience_for_next_level(const Mm3Game *g, const Mm3Character *ch) {
	return level_threshold(g, ch, ch->level);
}

uint32_t mm3_experience_needed(const Mm3Game *g, const Mm3Character *ch) {
	int32_t need = (int32_t)(mm3_experience_for_next_level(g, ch) - mm3_experience_total(g, ch));
	return need > 0 ? (uint32_t)need : 0;
}

void mm3_give_experience(const Mm3Game *g, uint32_t amount) {
	unsigned count = g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_PARTY_SIZE] : g->dg[MM3_DG_PARTY_STATE_BASE];
	if (!count) return;
	uint32_t share = amount / count;
	for (unsigned i = 0; i < count; i++) {
		unsigned index = g->dg[MM3_DG_ENGINE_MODE] == 2 ? g->dg[MM3_DG_COMBAT_ORDER + i] : i;
		mm3_party_member(g, index)->experience += share;
	}
}

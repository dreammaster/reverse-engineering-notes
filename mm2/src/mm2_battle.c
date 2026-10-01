#include "mm2_battle.h"

#include <string.h>

static int rnd(const Mm2Battle *b, int lo, int hi) {
	return b->rng.range(b->rng.ud, lo, hi);
}

int mm2_battle_visible(const Mm2Battle *b) {
	return b->count > MM2_VISIBLE_MONSTERS ? MM2_VISIBLE_MONSTERS : b->count;
}

static void init_slot(Mm2Battle *b, int slot) {
	const Mm2Monster *m = &b->table[b->id[slot]];
	b->status[slot] = 0;
	b->hp[slot] = (uint16_t)m->hp;
	b->speed[slot] = (uint8_t)m->speed;
	b->usesLeft[slot] = (uint8_t)m->specialUses;
}

/* TODO(review): ranks come from ovl/2COMBAT.asm combat_init_ranks (IDA 0x19640..0x19748) and the slots from
 * combat_init_monster(s) (0x195A8..0x1963F).  I could not map byte_1DC65's values to surprise with certainty from this
 * code alone: I took 2 = monsters surprised (their front rank halves), 3 = party surprised, matching the "You surprised
 * the monsters!" texts in combat_encounter (0x1A2A6).  Also unverified: the outdoor party rank branch for parties of 6+
 * (0x196EC..0x19714) was read from partially truncated disassembly. */
void mm2_battle_init(Mm2Battle *b, const Mm2Monster *table, const uint8_t *ids, int n, Mm2Char **party, int partySize,
					 int outdoors, Mm2Surprise surprise, const Mm2Rng *rng) {
	int i, slots;
	memset(b, 0, sizeof(*b));
	b->table = table;
	b->rng = *rng;
	b->outdoors = outdoors;
	b->surprise = surprise;
	b->partySize = partySize;
	for (i = 0; i < partySize && i < MM2_MAX_PARTY; i++)
		b->party[i] = party[i];
	b->count = n > MM2_MAX_MONSTERS ? MM2_MAX_MONSTERS : n;
	memcpy(b->id, ids, (size_t)b->count);

	/* monsters' front rank (byte_27815) */
	if (b->outdoors)
		b->frontMonsters = rnd(b, 10, 69) / 10 + partySize / 2;
	else
		b->frontMonsters = rnd(b, 10, 39) / 10 + 3;
	if (surprise == MM2_SURPRISE_MONSTERS) b->frontMonsters >>= 1;
	if (surprise == MM2_SURPRISE_PARTY) b->frontMonsters <<= 1;
	if (b->frontMonsters > b->count) b->frontMonsters = b->count;
	if (b->frontMonsters > 10) b->frontMonsters = 10;

	/* party's front rank (byte_22CED) */
	if (!b->outdoors) {
		b->frontParty = (rnd(b, 10, 79) / 10 + 3) >> 1;
	} else {
		b->frontParty = partySize;
		if (partySize >= 6)
			b->frontParty = (rnd(b, 10, 39) / 10 >> 1) + partySize - 2;
	}
	if (surprise == MM2_SURPRISE_PARTY) b->frontParty <<= 1;
	if (surprise == MM2_SURPRISE_MONSTERS) {
		if (partySize < 2) b->frontParty = 2;
		b->frontParty--;
	}
	if (b->frontParty > partySize) b->frontParty = partySize;

	slots = b->count > 10 ? 11 : b->count;
	for (i = 0; i < slots; i++)
		init_slot(b, i);
}

/* TODO(review): status wear-off at the start of a round (ovl/2COMBAT.asm combat_battle_loop IDA 0x1A15E..0x1A199): each
 * status bit is kept when the matching bit of rand(1, monsterId) is clear.  rand(1, id) with id 0 (monster 0) was not
 * checked against the original's rand_range for hi < lo. */
void mm2_battle_start_round(Mm2Battle *b) {
	int i, vis = mm2_battle_visible(b);
	memset(b->actedM, 0, sizeof(b->actedM));
	memset(b->actedP, 0, sizeof(b->actedP));
	for (i = 0; i < vis; i++) {
		int s = b->status[i] & 0xFE, r;
		if (s) {
			r = rnd(b, 1, b->id[i]);
			b->status[i] = (uint8_t)(((r & 0xFE) ^ 0xFE) & s);
		}
	}
}

Mm2ActorKind mm2_battle_next_actor(const Mm2Battle *b, int *index) {
	int i, vis = mm2_battle_visible(b), bestM = 0, mi = 0, bestP = 0, pi = 0;
	for (i = 0; i < vis; i++)
		if (!b->actedM[i] && b->speed[i] > bestM) {
			bestM = b->speed[i];
			mi = i;
		}
	for (i = 0; i < b->partySize; i++)
		if (!b->actedP[i]) {
			int sp = (int)mm2_c8(b->party[i], 0x6E);
			if (sp > bestP) {
				bestP = sp;
				pi = i;
			}
		}
	if (bestP && bestP >= bestM) {
		*index = pi;
		return MM2_ACTOR_PARTY;
	}
	if (bestM) {
		*index = mi;
		return MM2_ACTOR_MONSTER;
	}
	return MM2_ACTOR_NONE;
}

void mm2_battle_mark_acted(Mm2Battle *b, Mm2ActorKind kind, int index) {
	if (kind == MM2_ACTOR_PARTY) b->actedP[index] = 1;
	if (kind == MM2_ACTOR_MONSTER) b->actedM[index] = 1;
}

void mm2_battle_remove_monster(Mm2Battle *b, int slot) {
	int i;
	if (slot < 0 || slot >= b->count) return;
	for (i = slot; i < b->count - 1; i++) {
		b->id[i] = b->id[i + 1];
		if (i < 10) {
			b->status[i] = b->status[i + 1];
			b->hp[i] = b->hp[i + 1];
			b->speed[i] = b->speed[i + 1];
			b->usesLeft[i] = b->usesLeft[i + 1];
			b->actedM[i] = b->actedM[i + 1];
		}
	}
	b->count--;
	if (b->count >= 11) init_slot(b, 10);   /* a waiting monster steps into the spare slot */
	if (b->frontMonsters > b->count) b->frontMonsters = b->count;
}

static const uint8_t FLEE_TIER[4] = {3, 9, 24, 255};   /* DGROUP:1036, indexed by the record's verb field */

/* TODO(review): decision part of ovl/2COMBAT.asm combat_monster_turn (IDA 0x184FE..0x1866E) plus combat_monster_spell_roll
 * (0x1847E).  Assumptions:
 *  - the "outclassed monster flees" test uses DGROUP:1036[verb index] < byte_1E812 (party strength, set by
 *    combat_party_strength at 0x1974C) and byte_27814 (set near 0x1982x, meaning unknown; I call it "summoned");
 *    the text shown is " runs away!" from combat_monster_gone_text, but the exact meaning of those bytes is unverified;
 *  - the status >= 80h case (an "encased" monster casting a damage spell from tables at DGROUP:102A/1032, 0x18529) is not
 *    ported;
 *  - the spell-failed condition (spell id 15h..1Eh except 1Dh, and silenced or no-magic cell) was read at 0x18622..0x18648. */
Mm2MonsterAction mm2_monster_decide(const Mm2Battle *b, int slot, int partyStrength, int cellNoMagic, int summonedFlag) {
	const Mm2Monster *m = &b->table[b->id[slot]];
	int st = b->status[slot];
	int cast = 0;
	if (st & (MS_ENCASED | MS_HELD | MS_ASLEEP)) return MM2_MON_IDLE;
	if (!summonedFlag && FLEE_TIER[m->verb] < partyStrength && rnd(b, 1, 100) <= 50) return MM2_MON_FLEE;
	/* combat_monster_spell_roll (1847E): not mindless, uses left, d100 <= cast chance; uses a charge */
	{
		int r = rnd(b, 1, 100);
		if (!(st & MS_MINDLESS) && ((Mm2Battle *)b)->usesLeft[slot] && r <= m->castChancePct) {
			((Mm2Battle *)b)->usesLeft[slot]--;
			cast = 1;
		}
	}
	if (!cast) {
		if (slot < b->frontMonsters) return MM2_MON_MELEE;
		if (m->ranged && rnd(b, 1, 100) <= 80) return MM2_MON_RANGED;
		return MM2_MON_ADVANCE;
	}
	if (m->spell >= 0x0F && m->spell != 0x1D && m->spell < 0x1F && ((st & MS_SILENCED) || cellNoMagic))
		return MM2_MON_CAST_FAILED;
	return MM2_MON_CAST;
}

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

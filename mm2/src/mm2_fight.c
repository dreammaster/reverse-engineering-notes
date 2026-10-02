#include "mm2_fight.h"
#include "mm2_party.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int rnd(const Mm2Fight *f, int lo, int hi) {
	return f->b.rng.range(f->b.rng.ud, lo, hi);
}

static void logf_(Mm2Fight *f, const char *fmt, ...) {
	va_list ap;
	if (f->logCount == MM2_LOG_LINES) {   /* scroll */
		memmove(f->log[0], f->log[1], sizeof(f->log[0]) * (MM2_LOG_LINES - 1));
		f->logCount--;
	}
	va_start(ap, fmt);
	vsnprintf(f->log[f->logCount++], MM2_LOG_LEN, fmt, ap);
	va_end(ap);
}

static uint8_t *fx(Mm2Fight *f, unsigned dg) {
	return mm2_state_ptr((Mm2State *)f->roster->state, dg);
}

static Mm2Char *pc(Mm2Fight *f, int slot) {
	return &f->roster->chars[mm2_party_member(f->roster, slot)];
}

static const char *mname(const Mm2Fight *f, int slot) {
	return f->b.table[f->b.id[slot]].name;
}

/* TODO(review): ovl 2COMBAT combat_damage_character (IDA 0x17E10) halves damage with byte_1DC36 and (melee only) byte_1DC35,
 * then calls resident char_apply_damage (mm2.asm 0x13928).  For plain physical hits (element 0xFF, no resist flags) the latter
 * reduces to the logic below; the saving-throw branches for spells/elements are not ported. */
int mm2_char_take_damage(Mm2Char *c, int dmg) {
	uint8_t *x = c->raw;
	if (x[MC_CONDITION] >= 0x80) return 1;
	if (dmg <= 0) return x[MC_CONDITION] >= 0x40;
	x[MC_CONDITION] &= 0xEF;                       /* damage wakes a sleeper */
	if (x[MC_CONDITION] & 0x40) {
		x[MC_CONDITION] = 0x81;                   /* hit while unconscious: dead */
		x[MC_HP] = x[MC_HP + 1] = 0;
		return 1;
	}
	if ((int)mm2_c16(c, MC_HP) > dmg) {
		unsigned hp = mm2_c16(c, MC_HP) - (unsigned)dmg;
		x[MC_HP] = (uint8_t)hp;
		x[MC_HP + 1] = (uint8_t)(hp >> 8);
		return 0;
	}
	x[MC_CONDITION] |= 0x40;
	x[MC_HP] = x[MC_HP + 1] = 0;
	return 1;
}

void mm2_fight_start(Mm2Fight *f, Mm2Roster *roster, const Mm2Monster *table, const Mm2Item *items, const uint8_t *ids, int n,
					 Mm2Surprise surprise, const Mm2Rng *rng) {
	uint8_t list[MM2_MAX_MONSTERS];
	Mm2Char *party[MM2_MAX_PARTY];
	int i, cnt = 0;
	memset(f, 0, sizeof(*f));
	for (i = 0; i < n && cnt < MM2_MAX_MONSTERS; i++)
		if (ids[i]) list[cnt++] = ids[i];
	for (i = 0; i < mm2_party_size(roster) && i < MM2_MAX_PARTY; i++)
		party[i] = &roster->chars[mm2_party_member(roster, i)];
	f->roster = roster;
	f->items = items;
	mm2_battle_init(&f->b, table, list, cnt, party, mm2_party_size(roster), 0, surprise, rng);
	f->state = cnt ? MM2_FIGHT_RUNNING : MM2_FIGHT_VICTORY;
	if (surprise == MM2_SURPRISE_MONSTERS) logf_(f, "You surprised the monsters!");
	if (surprise == MM2_SURPRISE_PARTY) logf_(f, "The monsters surprised you!");
}

static int party_active(Mm2Fight *f) {
	int i;
	for (i = 0; i < f->b.partySize; i++)
		if (mm2_c8(f->b.party[i], MC_CONDITION) < 0x40) return 1;
	return 0;
}

/* TODO(review): victory: exp is divided among the survivors (cond < 80h) with the long divide at resident 0x...(res_0160 call at
 * ovl/2COMBAT.asm 0x19C4A); gold and gems are handed out later by the treasure screen (not ported), only experience and the
 * item list are produced here. */
static void finish_victory(Mm2Fight *f) {
	int i, alive = 0;
	f->state = MM2_FIGHT_VICTORY;
	if (f->loot.itemClass || f->loot.itemTier)
		f->nTreasure = mm2_treasure_roll(&f->loot, f->items, f->treasure, &f->b.rng);
	for (i = 0; i < f->b.partySize; i++)
		if (mm2_c8(f->b.party[i], MC_CONDITION) < 0x80) alive++;
	f->expEach = alive ? f->loot.exp / (uint32_t)alive : 0;
	for (i = 0; i < f->b.partySize; i++) {
		Mm2Char *c = f->b.party[i];
		if (mm2_c8(c, MC_CONDITION) < 0x80) {
			uint32_t e = mm2_c32(c, MC_EXP) + f->expEach;
			int k;
			for (k = 0; k < 4; k++)
				c->raw[MC_EXP + k] = (uint8_t)(e >> (8 * k));
		}
	}
	f->gold = f->loot.gold;
	logf_(f, "Victory!");
	logf_(f, "Each survivor receives %u experience", (unsigned)f->expEach);
}

static void check_over(Mm2Fight *f) {
	if (f->state != MM2_FIGHT_RUNNING) return;
	if (f->b.count == 0) finish_victory(f);
	else if (!party_active(f)) {
		f->state = MM2_FIGHT_DEFEAT;
		logf_(f, "Your party has been defeated.");
	}
}

static void kill_monster(Mm2Fight *f, int slot) {
	mm2_monster_reward(&f->loot, &f->b.table[f->b.id[slot]], f->b.id[slot], &f->b.rng);
	logf_(f, "%s goes down!", mname(f, slot));
	mm2_battle_remove_monster(&f->b, slot);
}

void mm2_fight_party_attack(Mm2Fight *f, int partySlot, int targetSlot, int shoot) {
	Mm2Char *c = pc(f, partySlot);
	Mm2AttackMods mods;
	Mm2AttackResult r;
	const Mm2Monster *m;
	if (targetSlot < 0 || targetSlot >= mm2_battle_visible(&f->b)) targetSlot = 0;
	m = &f->b.table[f->b.id[targetSlot]];
	mods.accuracyBonus = *fx(f, 0x3E3);
	mods.damageBonus = *fx(f, 0x3E7);
	mods.hitFloor = *fx(f, 0x3DB);
	r = mm2_party_attack(c, m->ac, shoot, &mods, &f->b.rng);
	mm2_battle_mark_acted(&f->b, MM2_ACTOR_PARTY, partySlot);
	logf_(f, "%.11s %s %s %d time%s", (const char *)c->raw, shoot ? "shoots" : "attacks", m->name, r.swings, r.swings == 1 ? "" : "s");
	if (!r.hits) {
		logf_(f, " and misses");
	} else {
		logf_(f, " hit %d for %d%s", r.hits, r.damage, r.kind == MM2_HIT_BACKSTAB ? " back stab" : r.kind == MM2_HIT_CRITICAL ? " critical" : "");
		f->b.status[targetSlot] |= MS_HURT;
		if (r.damage >= (int)f->b.hp[targetSlot]) {
			kill_monster(f, targetSlot);
		} else {
			f->b.hp[targetSlot] = (uint16_t)(f->b.hp[targetSlot] - r.damage);
			f->b.status[targetSlot] &= (uint8_t)~(MS_ASLEEP);   /* damage wakes a sleeping monster (assumed) */
		}
	}
	check_over(f);
}

/* TODO(review): spells in battle: only the damage spells decoded in mm2_spells.c are applied (to the chosen monster or the group
 * size the spell hits), without resistance rolls (combat_party_spell_hits, ovl/2COMBAT.asm 0x18696..0x188EF: magic resistance,
 * element immunity halving, status effects), SP/gem payment and the "spell failed" cases. */
void mm2_fight_party_cast(Mm2Fight *f, int partySlot, int spell, int targetSlot) {
	Mm2Char *c = pc(f, partySlot);
	Mm2CombatSpell cs;
	int dmg, i, hit = 0, n = mm2_battle_visible(&f->b);
	mm2_battle_mark_acted(&f->b, MM2_ACTOR_PARTY, partySlot);
	if (!mm2_combat_spell(spell, &cs)) {
		logf_(f, "%.11s's spell has no effect (not ported)", (const char *)c->raw);
		return;
	}
	logf_(f, "%.11s casts %s", (const char *)c->raw, MM2_SPELL_NAMES[spell]);
	dmg = mm2_combat_spell_damage(&cs, (int)mm2_c8(c, MC_LEVEL), &f->b.rng);
	if (targetSlot < 0 || targetSlot >= n) targetSlot = 0;
	for (i = targetSlot; i < n && hit < cs.targets; hit++) {
		const Mm2Monster *m = &f->b.table[f->b.id[i]];
		int d = dmg;
		if (cs.element > 0 && cs.element < 8 && m->immune[cs.element]) d >>= 1;
		logf_(f, " %s takes %d", m->name, d);
		if (d >= (int)f->b.hp[i]) {
			kill_monster(f, i);
			n = mm2_battle_visible(&f->b);   /* the next monster slid into slot i */
		} else {
			f->b.hp[i] = (uint16_t)(f->b.hp[i] - d);
			i++;
		}
	}
	check_over(f);
}

void mm2_fight_party_block(Mm2Fight *f, int partySlot) {
	mm2_battle_mark_acted(&f->b, MM2_ACTOR_PARTY, partySlot);
	logf_(f, "%.11s blocks", (const char *)pc(f, partySlot)->raw);
}

/* TODO(review): running (ovl/2COMBAT.asm combat_encounter pre-fight menu, IDA 0x1A2A6..0x1A77E, and combat_char_runs 0x1914A) uses
 * the map's run chance byte_231E3; I use a flat 40 % here. */
void mm2_fight_party_run(Mm2Fight *f, int partySlot) {
	mm2_battle_mark_acted(&f->b, MM2_ACTOR_PARTY, partySlot);
	if (rnd(f, 1, 100) <= 40) {
		f->state = MM2_FIGHT_FLED;
		logf_(f, "The party flees!");
	} else {
		logf_(f, "%.11s cannot get away", (const char *)pc(f, partySlot)->raw);
	}
}

static int pick_target(Mm2Fight *f, int rangedAnyRank) {
	int i, n = f->b.partySize, front = rangedAnyRank ? n : f->b.frontParty, live[MM2_MAX_PARTY], k = 0;
	if (front > n) front = n;
	for (i = 0; i < front; i++)
		if (mm2_c8(f->b.party[i], MC_CONDITION) < 0x40) live[k++] = i;
	if (!k)
		for (i = 0; i < n; i++)
			if (mm2_c8(f->b.party[i], MC_CONDITION) < 0x80) live[k++] = i;
	return k ? live[rnd(f, 0, k - 1)] : 0;
}

/* TODO(review): monster turn, from ovl/2COMBAT.asm combat_monster_turn (IDA 0x184FE) with combat_monster_melee (0x18398); target
 * choice (first living front-rank character vs random) is simplified to a random living front-rank character; "advances" just
 * grows the front rank by one; monster spells have no effect (combat_monster_casts 0x18056 not ported); touch effects of the
 * record (drain, poison ... combat_apply_touch_effect 0x1AFE2) are not applied. */
static void monster_turn(Mm2Fight *f, int slot) {
	const Mm2Monster *m = &f->b.table[f->b.id[slot]];
	Mm2MonsterAction act;
	mm2_battle_mark_acted(&f->b, MM2_ACTOR_MONSTER, slot);
	act = mm2_monster_decide(&f->b, slot, 0, 0, 1);
	switch (act) {
	case MM2_MON_IDLE:
		return;
	case MM2_MON_FLEE:
		logf_(f, "%s runs away!", m->name);
		mm2_battle_remove_monster(&f->b, slot);
		break;
	case MM2_MON_ADVANCE:
		logf_(f, "%s advances!", m->name);
		if (f->b.frontMonsters < mm2_battle_visible(&f->b)) f->b.frontMonsters++;
		break;
	case MM2_MON_CAST:
	case MM2_MON_CAST_FAILED:
		logf_(f, "%s casts a spell", m->name);
		break;
	case MM2_MON_MELEE:
	case MM2_MON_RANGED: {
		int t = pick_target(f, act == MM2_MON_RANGED);
		Mm2Char *c = f->b.party[t];
		Mm2MonsterAttackResult r;
		int dmg;
		r = mm2_monster_melee(m, f->b.id[slot] >> 4, (int)mm2_c8(c, MC_AC), f->b.status[slot] & MS_FRIGHTENED, f->b.status[slot] & MS_WEAKENED,
							  &f->b.rng);
		dmg = r.damage;
		if (*fx(f, 0x3E6)) dmg >>= 1;                       /* Power Shield */
		if (act == MM2_MON_MELEE && *fx(f, 0x3E5)) dmg >>= 1;  /* Shield protects against melee only */
		logf_(f, "%s %s %.11s", m->name, act == MM2_MON_RANGED ? "shoots" : "attacks", (const char *)c->raw);
		if (!r.hits) {
			logf_(f, " and misses");
		} else {
			int was = mm2_c8(c, MC_CONDITION) >= 0x40;
			logf_(f, " %d time%s for %d", r.hits, r.hits == 1 ? "" : "s", dmg);
			if (mm2_char_take_damage(c, dmg) && !was) logf_(f, "%.11s goes down!", (const char *)c->raw);
		}
		break;
	}
	}
	check_over(f);
}

Mm2ActorKind mm2_fight_next(Mm2Fight *f, int *index) {
	int emptyRounds = 0;
	while (f->state == MM2_FIGHT_RUNNING) {
		Mm2ActorKind k;
		int idx;
		if (!f->roundStarted) {
			mm2_battle_start_round(&f->b);
			f->roundStarted = 1;
		}
		k = mm2_battle_next_actor(&f->b, &idx);
		if (k == MM2_ACTOR_NONE) {
			f->roundStarted = 0;
			if (++emptyRounds > 2) {   /* nobody can act any more */
				f->state = MM2_FIGHT_DEFEAT;
				break;
			}
			continue;
		}
		emptyRounds = 0;
		if (k == MM2_ACTOR_MONSTER) {
			monster_turn(f, idx);
			continue;
		}
		if (mm2_c8(f->b.party[idx], MC_CONDITION) >= 0x40 || (mm2_c8(f->b.party[idx], MC_CONDITION) & 0x30)) {
			mm2_battle_mark_acted(&f->b, MM2_ACTOR_PARTY, idx);   /* unconscious, asleep or paralysed members skip their turn */
			continue;
		}
		*index = idx;
		return k;
	}
	return MM2_ACTOR_NONE;
}

/* A fight: ties the battle state, the attack formulas, damage application, kills, loot and the victory/defeat checks
 * together (docs/combat.md).  No user interface: the caller asks who acts (mm2_fight_next), feeds party actions and
 * reads the text log. */
#ifndef MM2_FIGHT_H
#define MM2_FIGHT_H

#include "mm2_battle.h"
#include "mm2_inn.h"
#include "mm2_reward.h"
#include "mm2_spells.h"

#define MM2_LOG_LINES 24
#define MM2_LOG_LEN 64

typedef enum { MM2_FIGHT_RUNNING, MM2_FIGHT_VICTORY, MM2_FIGHT_DEFEAT, MM2_FIGHT_FLED } Mm2FightState;

typedef struct {
	Mm2Battle b;
	Mm2Roster *roster;
	const Mm2Item *items;
	Mm2Loot loot;
	Mm2FightState state;
	char log[MM2_LOG_LINES][MM2_LOG_LEN];
	int logCount;
	Mm2TreasureItem treasure[3];
	int nTreasure;
	uint32_t expEach;     /* experience each survivor receives after a victory */
	uint32_t gold;        /* loot gold split among the party (sum, after victory) */
	int battleNumber;
	/* the current round */
	int roundStarted;
} Mm2Fight;

/* Starts a fight against the monsters `ids[0..n)` (0 entries are ignored), the party being the roster's party. */
void mm2_fight_start(Mm2Fight *f, Mm2Roster *roster, const Mm2Monster *table, const Mm2Item *items, const uint8_t *ids, int n,
					 Mm2Surprise surprise, const Mm2Rng *rng);

/* Who acts next; starts a new round when nobody is left.  Monster turns are executed here and a party member is only
 * reported (MM2_ACTOR_PARTY, index = party slot); returns MM2_ACTOR_NONE when the fight is over. */
Mm2ActorKind mm2_fight_next(Mm2Fight *f, int *index);

/* Party actions for the slot returned by mm2_fight_next.  Each marks the member as having acted. */
void mm2_fight_party_attack(Mm2Fight *f, int partySlot, int targetSlot, int shoot);
void mm2_fight_party_cast(Mm2Fight *f, int partySlot, int spell, int targetSlot);
void mm2_fight_party_block(Mm2Fight *f, int partySlot);
/* Run: succeeds with the chance of a d100 against 30 + ... (see the TODO in the .c file). */
void mm2_fight_party_run(Mm2Fight *f, int partySlot);

/* Applies physical damage to a character like char_apply_damage (0x13928): sleep wakes, 0 HP means unconscious, a hit on
 * an unconscious character kills.  Returns 1 if the character is now out of action. */
int mm2_char_take_damage(Mm2Char *c, int dmg);

#endif

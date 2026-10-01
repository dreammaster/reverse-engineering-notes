/* Battle state and turn order (docs/combat.md; 2COMBAT combat_init_*, combat_battle_loop 1A0D4). */
#ifndef MM2_BATTLE_H
#define MM2_BATTLE_H

#include "mm2_combat.h"

#define MM2_MAX_MONSTERS 255
#define MM2_VISIBLE_MONSTERS 10
#define MM2_MAX_PARTY 8

enum {
	MS_HURT = 0x01, MS_SILENCED = 0x02, MS_WEAKENED = 0x04, MS_FRIGHTENED = 0x08,
	MS_ASLEEP = 0x10, MS_HELD = 0x20, MS_MINDLESS = 0x40, MS_ENCASED = 0x80
};

typedef enum { MM2_SURPRISE_NONE = 0, MM2_SURPRISE_MONSTERS = 2, MM2_SURPRISE_PARTY = 3 } Mm2Surprise;

typedef struct {
	const Mm2Monster *table;           /* MONSTERS.DAT records by id */
	uint8_t id[MM2_MAX_MONSTERS];      /* monsters in order; the first 10-11 are in the slots, the rest wait */
	int count;                         /* word_1DD58: monsters still alive or waiting */
	/* slot state (index = position in the visible list) */
	uint8_t status[11];
	uint16_t hp[11];
	uint8_t speed[11];
	uint8_t usesLeft[11];
	uint8_t actedM[11];
	uint8_t actedP[MM2_MAX_PARTY];
	Mm2Char *party[MM2_MAX_PARTY];
	int partySize;
	int frontMonsters;                 /* byte_27815: monsters in the front rank */
	int frontParty;                    /* byte_22CED */
	int outdoors;
	Mm2Surprise surprise;
	Mm2Rng rng;
} Mm2Battle;

/* Sets up a battle (combat_init_ranks + combat_init_monsters).  ids: the monsters of the encounter. */
void mm2_battle_init(Mm2Battle *b, const Mm2Monster *table, const uint8_t *ids, int n, Mm2Char **party, int partySize,
					 int outdoors, Mm2Surprise surprise, const Mm2Rng *rng);

/* Start of a round: clears the "acted" flags and lets status bits wear off. */
void mm2_battle_start_round(Mm2Battle *b);

typedef enum { MM2_ACTOR_NONE, MM2_ACTOR_PARTY, MM2_ACTOR_MONSTER } Mm2ActorKind;

/* Picks the next actor: the fastest ready party member acts when its speed >= the fastest ready monster's. */
Mm2ActorKind mm2_battle_next_actor(const Mm2Battle *b, int *index);
void mm2_battle_mark_acted(Mm2Battle *b, Mm2ActorKind kind, int index);

/* Removes monster `slot` (killed) and refills from the waiting list. */
void mm2_battle_remove_monster(Mm2Battle *b, int slot);

/* Number of monsters currently in the visible list (min(count, 10)). */
int mm2_battle_visible(const Mm2Battle *b);

#endif

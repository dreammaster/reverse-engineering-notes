#ifndef ULTIMA6_ULTIMA6_CREATURES_H
#define ULTIMA6_ULTIMA6_CREATURES_H

#include "engine/random.h"
#include "ultima6/actors.h"
#include "ultima6/combat.h"
#include "ultima6/movement.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The monsters and animals of the world. They are hatched from eggs (objects of type 335 whose contents are the creatures to
 * make): when the party comes within 20 squares of an egg that is not yet active it becomes active and, if the hour suits its
 * quality (below 10 always, 10-19 daytime, 20-29 night, 30 and up always) and a random 1-100 is within its quantity (or the
 * quantity is 100), creates 1 to n creatures of each kind it holds, as temporary actors with numbers from 0xE0 up. An egg
 * becomes inactive again when the party is 20 or more squares away. Creatures that are evil or chaotic hunt the party:
 * they walk towards the nearest member and fight it when next to it; the others wander. Temporary actors far from the party
 * disappear.
 */
class Creatures {
public:
	enum { FIRST_TEMP_ACTOR = 0xE0, EGG = 335, SPAWN_RANGE = 20, HUNT_RANGE = 12, DESPAWN_RANGE = 40, FIRST_CREATURE = 342 };

	Creatures(World &world, const Movement &movement, ActorManager &actors, Combat &combat, BorlandRandom &rng, TextWindow &scroll)
		: _world(world), _movement(movement), _actors(actors), _combat(combat), _rng(rng), _scroll(scroll) {
	}

	/** Checks the eggs around the party (call after the party moved). */
	void checkEggs(int px, int py, int pz, int hour);
	/** The companions' blows (once a minute) and the disappearing of far creatures. */
	void partyFights();
	void despawn(int px, int py, int pz);
	/** One action of a creature the turn scheduler picked. */
	void act(int actor, int px, int py, int pz);
	/** A slime that has been hurt and lives divides. */
	void divide(int actor);
	/** sub_2A4B6: where the two sides stand, worked out once at the start of every round of turns. */
	void formation(int px, int py, int pz);

	/** The person who walked up to the party to speak to it (worktypes 0x96, 0x97), or -1; asking clears it. */
	int takeTalkRequest() {
		int r = _talkRequest;
		_talkRequest = -1;
		return r;
	}

	int count() const; // living temporary actors

	int spawn(int type, int worktype, int x, int y, int z);

private:
	bool stepDir(int actor, int dir);
	bool stepDiag(int actor, int vertical, int horizontal);
	bool stepToward(int actor, int tx, int ty);
	int pickTarget(int actor, bool strongest = false);
	void turn(int actor, int px, int py);
	void assault(int actor, int px, int py);
	void wander(int actor, int px, int py);
	void work(int actor);
	int workApproach(int actor);
	void wake(int actor);
	void combatFront(int actor, int px, int py);
	void combatRear(int actor, int px, int py);
	void combatFlank(int actor, int px, int py);
	bool formationWalk(int actor, int px, int py);
	int actorAt(int x, int y) const;
	bool validTarget(int actor, int target) const;
	bool lineOfFire(int actor, int tx, int ty, int range) const;
	void scan();
	void hatch(int egg, int hour);

	World &_world;
	const Movement &_movement;
	ActorManager &_actors;
	Combat &_combat;
	BorlandRandom &_rng;
	TextWindow &_scroll;
	/** word_4C435..4C44B: the sides' centres and the actors at the ends of the lines */
	struct Formation {
		int partyX = 0, partyY = 0, enemyX = 0, enemyY = 0;
		int partyFront = 0, partyRight = 0, partyLeft = 0;
		int enemyRight = -1, enemyLeft = -1, lead = -1;
		int enemies = 0;
	} _f;
	int _winX = 0, _winY = 0, _fz = 0; // terrain_cache_x/y and the party's level
	int _talkRequest = -1;
	bool _revealed = false; // word_3F1B8
	bool _shuffle = false;             // word_3F002: party members may share a square while the line forms
	std::vector<int> _eggs;
	std::vector<bool> _active;
	bool _scanned = false;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

#ifndef ULTIMA6_ULTIMA6_COMBAT_H
#define ULTIMA6_ULTIMA6_COMBAT_H

#include "engine/random.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <string>

namespace Ultima6 {
namespace Ultima6 {

struct CreatureStats {
	int type, str, dex, intel, hp, dmg, align, flags, exp;

	enum Flag { // bits of `flags`: the original's flag number minus 7
		CAN_TALK = 1 << 0,
		BLEEDS = 1 << 1,
		FADES_AWAY = 1 << 2,
		LIVES_IN_WATER = 1 << 5,
		POISONOUS = 1 << 8,
		STRENGTH_BASED = 1 << 9,
		UNDEAD = 1 << 13
	};
	enum Alignment { GOOD, NEUTRAL, EVIL, CHAOTIC };
};

/**
 * Hand-to-hand fighting by the rules of the original's creature, weapon and armour tables (see combat_tables.inc):
 * a hit is decided by the attacker's strength (for clubs, maces and hammers and strength based creatures) or dexterity against the
 * defender's dexterity, the damage is a random 1..weapon damage (a creature's own damage when unarmed) less a random
 * 1..armour class of what the defender wears, and what is left comes off the hit points. Kills of evil creatures earn
 * experience. Ranged weapons, spells and the creatures' own turns are not done yet.
 */
class Combat {
public:
	Combat(World &world, BorlandRandom &rng, TextWindow &scroll) : _world(world), _rng(rng), _scroll(scroll) {
	}

	static const CreatureStats *stats(int type);
	static int weaponDamageOf(int type); // -1 if the object type is no weapon
	static int armourOf(int type);

	/** The weapon an actor fights with: the best readied weapon's type, or its own type when unarmed. */
	int weaponType(int actor) const;
	int armourClass(int actor) const;
	std::string nameOf(int actor) const;

	/** Takes `damage` hit points off the defender (spells and traps too): prints how badly it is hurt, kills it and rewards the attacker. */
	bool applyDamage(int attacker, int defender, int damage);
	int randomBetween(int lo, int hi) {
		return random(lo, hi);
	}
	/** One attack; prints what happened. True if the defender died. */
	bool attack(int attacker, int defender);

private:
	bool hitCheck(int attacker, int defender, int weapon);
	void die(int actor);
	int random(int lo, int hi) {
		return lo + _rng.next() % (hi - lo + 1);
	}

	World &_world;
	BorlandRandom &_rng;
	TextWindow &_scroll;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

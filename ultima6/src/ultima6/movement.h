#ifndef ULTIMA6_ULTIMA6_MOVEMENT_H
#define ULTIMA6_ULTIMA6_MOVEMENT_H

#include "ultima6/world.h"

#include <cstdint>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The movement classes of GAME.EXE: a table in the code segment (seg066) that lists the creature
 * types with their own rules (bats fly, sea serpents swim, ghosts walk through things ...), and one
 * flag word per class. Types that are not listed use class -1, whose word is the one that precedes
 * the flag table.
 */
struct MovementClasses {
	enum {
		COUNT = 64,
		FLIES = 0x100,        // may cross terrain that does not block flight
		SWIMS = 0x200,        // stays on water
		IGNORES_OBJECTS = 0x400, // walks through scenery
		OPENS_DOORS = 0x4000, // may pass doors, portcullises and the like
		UNLISTED_FLAGS = 0x6000
	};

	static int classOf(int type);
	static uint16_t flags(int cls);
};

/**
 * What the original's passability check (sub_25D7F) consults beyond the map: a handful of globals.
 */
struct MoveRules {
	bool avoidDamage = false;    // word_3F004: refuse tiles that hurt
	bool partyMayShuffle = true; // word_3F002: party members may swap places while walking
	bool questFlag = false;      // word_48F65, the first byte of the saved globals
	bool deniedPending = true;   // byte_3EFFD: the "Thou art not upon a Sacred Quest" text is still due
	int leader = 1;              // actor number of the party member under control (ds:3533[byte_404A4])
	const std::vector<uint8_t> *actorFlags = nullptr; // per-actor movement flags (ds far array), bit 0x10 = will not swap
};

/** The outcome of a passability check, with the side effects the original leaves in globals. */
struct MoveResult {
	bool passable = false;
	bool damaging = false; // word_3EFF9: the target tile hurts
	bool denied = false;   // the Sacred Quest guard stopped the party; the message was due
};

/**
 * Port of sub_25D7F: can an object (normally an actor) enter a map position? Terrain is judged by the
 * mover's movement class, then every object on the position (including the parts of two- and
 * four-tile objects that reach into it) may block, release, or hand the mover to the next object.
 */
class Movement {
public:
	explicit Movement(const World &world) : _world(world) {
	}

	/** mover is an index into World::objects. */
	MoveResult check(int x, int y, int z, int mover, const MoveRules &rules) const;

	/** Objects that reach into a cell, with how far into the object's own tile set the cell lies (0..3). */
	struct Touch {
		int object;
		int part;
	};
	void touching(int x, int y, int z, std::vector<Touch> &out) const;

private:
	const World &_world;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

#ifndef ULTIMA6_ULTIMA6_ACTORS_H
#define ULTIMA6_ULTIMA6_ACTORS_H

#include "ultima6/movement.h"
#include "ultima6/world.h"

namespace Ultima6 {
namespace Ultima6 {

struct GameClock;

/**
 * The people of the world living their schedules. Once an hour update_actor_schedules (GAME.EXE 2AED5)
 * looks through the SCHEDULE entries of every actor (2-223) that is not in the party and has no task of its
 * own (worktype 0x80 and up, or 7): the last entry whose hour is the game hour and whose day of the week
 * matches (or is 0, every day) is stored as the actor's schedule index and the worktype becomes 0x81, "go to
 * your place". Every minute those actors walk towards the entry's position when it is near the party (inside
 * the 40 x 40 square the game keeps in memory) and are put there at once when it is far, and then take up
 * the entry's worktype.
 *
 * The walking here is a breadth-first search of up to a 41 x 41 square over the Movement rules; the original
 * uses its own path finder (sub_2868C).
 */
class ActorManager {
public:
	enum { GO_TO_SCHEDULE = 0x81, NEAR = 19 };

	ActorManager(World &world, const Movement &movement) : _world(world), _movement(movement) {
	}

	void hourChanged(const GameClock &clock);
	/** One game minute; (px, py, pz) is the party's position. */
	void tick(int px, int py, int pz);
	/** The schedule-walkers' part of every turn (sub_2A8DA / 2A3BA): actors that have reached their place take up its worktype, and those far from the party are put there. */
	void scheduleUpdate(int px, int py, int pz);
	/** One action of a non-party actor 2-0xDF that was chosen by the turn scheduler: walks towards its schedule place when it is going there, otherwise does nothing; always uses up moves. */
	void act(int actor, int px, int py, int pz);

	/** Sets the frame of an actor's sprite for a step in direction 0-7 (0 north, 1 northeast, 2 east, 3 southeast, 4 south ... 7 northwest), sub_263D4. */
	void face(int actor, int direction);
	/** The direction 0-7 of a step by (dx, dy), -1 for none. */
	static int directionOf(int dx, int dy);

	/** One breadth-first step of an actor towards a square; false if there is no way. */
	bool walkToward(int actor, int objIndex, int tx, int ty, int z) {
		return step(actor, objIndex, tx, ty, z);
	}

private:
	bool step(int actor, int objIndex, int tx, int ty, int z);
	bool squareShut(int x, int y, int z) const;
	bool inWindow(int x, int y, int z, int px, int py, int pz, int &gx, int &gy) const;
	bool plan(int actor, int tx, int ty, int px, int py, int pz);
	void follow(int actor, int destX, int destY, int px, int py, int pz);
	void arrive(int actor, int px, int py, int pz);

	World &_world;
	const Movement &_movement;
	int _walkFrame[ObjList::ACTORS] = {};
	int _owner[8] = {}, _index[8] = {}, _rep[8] = {};
	uint8_t _path[8][32] = {};
};

} // namespace Ultima6
} // namespace Ultima6

#endif

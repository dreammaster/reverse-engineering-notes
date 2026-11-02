#ifndef ULTIMA6_ULTIMA6_SESSION_H
#define ULTIMA6_ULTIMA6_SESSION_H

#include "engine/backend.h"
#include "ultima6/map_renderer.h"
#include "ultima6/movement.h"
#include "ultima6/world.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The game clock as the saved game keeps it (objlist +0x1BF3..): minute, hour, day (1-28), month
 * (1-13) and year; GAME.EXE's time advance (sub_13305) steps it by minutes.
 */
struct GameClock {
	int minute, hour, day, month, year;

	void advance(int minutes);
	/** Day of the week 1-7 as the schedule check computes it: (day - 1) % 7 + 1. */
	int dayOfWeek() const {
		return (day - 1) % 7 + 1;
	}
};

/**
 * The running game: party position, clock, message log and the commands. Movement and time follow the
 * original in structure (one step is one minute; directions are the eight-way table at ds:01A2 / 01AA
 * with north first, clockwise); walkability is the port of GAME.EXE's sub_25D7F in Movement.
 */
class Game {
public:
	explicit Game(World &world);

	/** Handles one key; false when the game should end. */
	bool handleKey(int key);
	void render(Framebuffer &screen);

	/** Moves the party leader one step in a direction (0 = north, clockwise); false if blocked. */
	bool step(int dir);
	/** Whether the party leader can enter a tile. */
	bool canWalk(int x, int y, int z);

	void message(const std::string &text);

	int x, y, z;
	GameClock clock;
	std::vector<std::string> log;

private:
	World &_world;
	MapRenderer _renderer;
	Movement _movement;
	MoveRules _rules;
	int _leaderObject; // index of the leader's body in the world's object list
};

} // namespace Ultima6
} // namespace Ultima6

#endif

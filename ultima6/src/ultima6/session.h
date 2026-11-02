#ifndef ULTIMA6_ULTIMA6_SESSION_H
#define ULTIMA6_ULTIMA6_SESSION_H

#include "engine/backend.h"
#include "ultima6/map_renderer.h"
#include "ultima6/movement.h"
#include "ultima6/actors.h"
#include "ultima6/combat.h"
#include "ultima6/creatures.h"
#include "ultima6/magic.h"
#include "ultima6/talk.h"
#include "ultima6/text_window.h"
#include "ultima6/world.h"

#include <memory>
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

	/** Lets game time pass: the clock, the hourly schedules and the people walking about. */
	void advanceTime(int minutes);
	/** sub_2AB7A: runs the actors' turns after a command until it is the player's turn again; a game minute passes whenever nobody has moves left. */
	void runTurns();

	/** Writes the saved game (the world and the clock) to `saveDir`. */
	bool save();
	std::string saveDir;

	void message(const std::string &text);
	/** Prints the command prompt: the name of the party member in control, a colon, and '>'. */
	void prompt();

	int x, y, z;
	GameClock clock;
	std::vector<std::string> log;
	TextWindow scroll; // the text scroll of the play screen

private:
	World &_world;
	MapRenderer _renderer;
	Movement _movement;
	ActorManager _actors;
	MoveRules _rules;
	BorlandRandom _rng;
	Combat _combat;
	Creatures _creatures;
	Magic _magic;
	const Magic::SpellInfo *_spell = nullptr; // the spell being cast, waiting for its target
	std::string _invocation;                  // circle digit and syllables typed so far
	void castKey(int key);
	bool useHorse(int tx, int ty);
	bool handleKeyInner(int key);
	void minutePassed();
	bool boardOrLeave(int tx, int ty);
	bool _aboard = false;     // the party is on a skiff or raft: the leader's body has the vehicle's type
	int _footType = 0;        // the leader's own type while aboard
	int _footFrame = 0;
	void partyHazards();
	void castAt(int tx, int ty);
	void attackAt(int tx, int ty);
	std::unique_ptr<Talk> _talk; // a conversation in progress
	int _cx = 5, _cy = 5;        // cursor cell in the map view while targeting with it
	bool _cursor = false;
	void look(int tx, int ty);
	int _pending = 0;            // command letter waiting for its target (a direction), or 0
	bool selectTarget(int key, int &tx, int &ty) const;
	void command(int key);
	void talkAt(int tx, int ty);
	void getAt(int tx, int ty);
	void useAt(int tx, int ty);
	void changeLevel(int nx, int ny, int nz);
	void restKey(int key);
	void followLeader(int oldX, int oldY);
	bool _confirmSave = false;
	bool _firstPrompt = true;
	bool _inventory = false; // the panel shows the inventory instead of the statistics
	int _selected = 0;
	std::vector<int> _items;
	bool inventoryKey(int key);
	bool _dead = false;
	std::string _number; // digits typed for the Rest command
	int _member = 0; // party member shown in the panel
	int _leaderObject; // index of the leader's body in the world's object list
};

} // namespace Ultima6
} // namespace Ultima6

#endif

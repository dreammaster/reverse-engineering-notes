// Which track goes with the place (sub_377C8): combat ENGAGE, a ship HORNPIPE, the surface one of four, any other level DUNGEON (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond, what)                                                        \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, what);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	int m = g.music();
	CHECK(m == 8 || m == 1 || m == 5 || m == 10, "on the surface: STONES, ULTIMA, FOREST or BRIT");
	g.handleKey('b'); // Begin combat!
	CHECK(g.music() == 7, "combat is ENGAGE");
	g.handleKey('b'); // Break off combat!
	m = g.music();
	CHECK(m == 8 || m == 1 || m == 5 || m == 10, "and the surface music again");
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

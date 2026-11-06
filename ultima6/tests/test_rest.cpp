// Resting in the wilderness (cmd_rest): the camp, the meal, the guard, the healing (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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
	// open ground with no building within six squares, where the party can camp
	for (int yy = 560; yy < 900; yy += 7)
		for (int xx = 560; xx < 900; xx += 7) {
			bool open = true;
			for (int dy = -2; dy <= 2; ++dy)
				for (int dx = -2; dx <= 2; ++dx) {
					int t = w.map.tile(0, xx + dx, yy + dy);
					std::vector<int> there;
					w.objectsAt(xx + dx, yy + dy, 0, there);
					open &= !(w.flags.flags1[t & 2047] & 3) && there.empty();
				}
			for (int dy = -6; dy <= 6; ++dy)
				for (int dx = -6; dx <= 6; ++dx) {
					int f = w.flags.flags1[w.map.tile(0, xx + dx, yy + dy) & 2047];
					if ((f & 4) && (f & 0xF0))
						open = false;
				}
			if (!open)
				continue;
			for (int m = 0; m < w.actors.partySize; ++m)
				w.moveObject(w.actorObject(w.actors.partyRoster[m]), xx, yy, 0);
			g.x = xx;
			g.y = yy;
			int buddy = w.actors.partyRoster[2];
			w.actors.hp[buddy] = 5;
			int hour = g.gameClock.hour;
			g.handleKey('R');
			g.handleKey('5');
			g.handleKey('2');
			std::string all;
			for (const std::string &l : g.log)
				all += l;
			CHECK(all.find("Mealtime!") != std::string::npos, "Mealtime");
			CHECK(all.find("Dupre has food.") != std::string::npos, "the names are whole");
			CHECK(all.find("Dupre stands guard while the party rests.") != std::string::npos, "the guard");
			CHECK(all.find("has healed.") != std::string::npos || all.find("You hear something!") != std::string::npos, "healed or heard something");
			CHECK(w.actors.hp[buddy] > 5 || all.find("You hear something!") != std::string::npos, "the party mended");
			CHECK(g.gameClock.hour != hour, "time passed");
			if (failures == 0)
				std::printf("ok\n");
			return failures ? 1 : 0;
		}
	std::printf("skip (no open ground)\n");
	return 0;
}

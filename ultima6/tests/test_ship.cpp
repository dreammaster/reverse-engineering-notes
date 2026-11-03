// Boarding and sailing a three-part ship (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
#include <string>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static std::string logText(const Game &g) {
	std::string s;
	for (const std::string &l : g.log)
		s += l;
	return s;
}

// the squares of the three parts of the ship whose middle is (mx, my)
static int shipAt(const World &w, int x, int y) {
	for (size_t i = 0; i < w.objects.size(); ++i) {
		const World::Obj &o = w.objects[i];
		if (!o.deleted && o.type == 0x19C && o.x == x && o.y == y && o.onMap())
			return (int)i;
	}
	return -1;
}

int main() {
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	// the ship at 316,443-445 (frames 1, 9, 17: heading north; the middle is ship number 7)
	int mid = shipAt(w, 316, 444);
	CHECK(mid >= 0 && w.objects[mid].frame == 9 && w.objects[mid].quality == 7);
	if (mid < 0)
		return 1;
	// stand on the first walkable square next to the middle: east or west
	int px = 317;
	if (!g.canWalk(px, 444, 0))
		px = 315;
	w.moveObject(w.actorObject(1), px, 444, 0);
	g.x = px;
	g.y = 444;
	g.z = 0;
	int toward = px == 317 ? KEY_LEFT : KEY_RIGHT;
	// without the deed
	g.handleKey('u');
	g.handleKey(toward);
	CHECK(logText(g).find("A deed is required.") != std::string::npos);
	CHECK(shipAt(w, 316, 444) >= 0);
	// with it
	w.addToInventory(1, 0x95, 7, 1);
	g.handleKey('u');
	g.handleKey(toward);
	CHECK(logText(g).find("Boarded.") != std::string::npos);
	CHECK(shipAt(w, 316, 444) < 0 || w.objects[shipAt(w, 316, 444)].actor >= 0); // the middle is now the party's
	CHECK(g.x == 316 && g.y == 444);
	// sail north as far as the water allows, then check the ship has stayed in one piece
	int moved = 0;
	for (int i = 0; i < 6; ++i)
		moved += g.step(0);
	std::printf("sailed %d squares north, now at %d,%d\n", moved, g.x, g.y);
	int bow = shipAt(w, g.x, g.y - 1), stern = shipAt(w, g.x, g.y + 1);
	CHECK(bow >= 0 && w.objects[bow].frame == 1);
	CHECK(stern >= 0 && w.objects[stern].frame == 17);
	// turn east: the ends swing round
	if (g.step(2)) {
		int b2 = shipAt(w, g.x + 1, g.y), s2 = shipAt(w, g.x - 1, g.y);
		CHECK(b2 >= 0 && w.objects[b2].frame == 3);
		CHECK(s2 >= 0 && w.objects[s2].frame == 19);
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

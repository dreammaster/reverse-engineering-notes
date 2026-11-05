// Solo mode (the digit keys of command_loop): one member walks alone, the others stand still; '0' brings the party together again (skipped when the game data is absent).

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

int main() {
	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	ObjList &a = w.actors;
	CHECK(a.partySize >= 3);
	int second = a.partyRoster[1], third = a.partyRoster[2];
	int k1 = w.actorObject(second), k2 = w.actorObject(third);

	g.handleKey('3'); // the third member walks alone
	CHECK(logText(g).find("Solo mode") != std::string::npos);
	CHECK(g.x == w.objects[k2].x && g.y == w.objects[k2].y);
	CHECK(a.worktype[third] == 2 && a.worktype[second] == 0); // the follower stands still

	// walking moves that member only
	int ax = w.objects[w.actorObject(1)].x, ay = w.objects[w.actorObject(1)].y;
	int sx = w.objects[k1].x, sy = w.objects[k1].y;
	int tx = w.objects[k2].x, ty = w.objects[k2].y;
	bool moved = false;
	for (int dir : {KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT}) {
		g.handleKey(dir);
		moved = w.objects[k2].x != tx || w.objects[k2].y != ty;
		if (moved)
			break;
	}
	CHECK(moved);
	CHECK(w.objects[w.actorObject(1)].x == ax && w.objects[w.actorObject(1)].y == ay);
	CHECK(w.objects[k1].x == sx && w.objects[k1].y == sy);

	// the commands that are not for a lone walker
	g.log.clear();
	g.handleKey('b');
	CHECK(logText(g).find("Not in solo mode!") != std::string::npos);

	// party mode again (everyone is within eight squares of the walker)
	g.log.clear();
	g.handleKey('0');
	CHECK(logText(g).find("Party mode") != std::string::npos);
	CHECK(a.worktype[a.partyRoster[0]] == 2 && a.worktype[second] == 1);

	// nobody can be made to walk alone aboard a ship or in combat: here, in combat
	g.handleKey('b');
	g.log.clear();
	g.handleKey('2');
	CHECK(logText(g).find("Not in combat mode!") != std::string::npos);

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}

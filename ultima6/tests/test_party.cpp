// The party's followers keep to the formation behind the leader (sub_26F03; skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
#include <cstdlib>

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

int main() {
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	const ObjList &a = w.actors;
	CHECK(a.partySize >= 3);
	// the members stand together at the start; walk east and north and see them keep up, one to a square
	for (int i = 0; i < 8; ++i)
		g.step(i < 4 ? 2 : 0);
	int lx = g.x, ly = g.y;
	bool stacked = false;
	for (int m = 1; m < a.partySize; ++m) {
		int k = w.actorObject(a.partyRoster[m]);
		int dx = std::abs((int)w.objects[k].x - lx), dy = std::abs((int)w.objects[k].y - ly);
		std::printf("member %d is %d,%d from the leader\n", m, dx, dy);
		CHECK(dx <= 5 && dy <= 5);
		for (int n = m + 1; n < a.partySize; ++n) {
			int k2 = w.actorObject(a.partyRoster[n]);
			stacked |= w.objects[k].x == w.objects[k2].x && w.objects[k].y == w.objects[k2].y;
		}
	}
	CHECK(!stacked);
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

// Sound effects the game asks for (sub_36E85 call sites): a step that is blocked plays the tone 0x137 of effect 1; a blow that is not a missile makes effect 0x11
// when it is struck near the party (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
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
	std::vector<std::pair<int, int>> heard;
	g.sound = [&](int effect, int param) { heard.push_back({effect, param}); };

	// walk in each direction in turn until a step is refused
	bool blocked = false;
	for (int dir = 0; dir < 8 && !blocked; dir += 2) {
		for (int tries = 0; tries < 300 && !blocked; ++tries) {
			heard.clear();
			if (!g.step(dir)) {
				blocked = true;
				CHECK(heard.size() == 1 && heard[0].first == 1 && heard[0].second == 0x137, "a refused step plays effect 1 with 0x137");
			}
		}
	}
	CHECK(blocked, "something blocked the way");
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

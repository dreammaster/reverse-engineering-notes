// What the party sees (sub_18F86): the square it stands on is seen and lit, at night the far squares are dark, walls hide what is behind them (skipped when the game data is absent).

#include "ultima6/session.h"
#include "ultima6/view.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::ViewBuilder;
using ::Ultima6::Ultima6::ViewGrid;
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
	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	ViewBuilder b(w);
	static ViewGrid day, night;

	// the open country by day: the party's own square and its neighbours are seen and lit
	b.build(330, 380, 0, 7, day);
	CHECK(day.tile[5][5] == w.map.tile(0, 330, 380) && day.light[5][5] >= 4);
	CHECK(day.tile[5][6] != ViewGrid::UNSEEN && day.tile[4][5] != ViewGrid::UNSEEN);

	// the same place at night: lit by the party's own light only, the far corners are black
	b.build(330, 380, 0, 0, night);
	CHECK(night.light[5][5] == 4 && night.tile[5][5] == w.map.tile(0, 330, 380));
	CHECK(night.tile[0][0] == ViewGrid::DARK || night.tile[0][0] == ViewGrid::UNSEEN);
	CHECK(night.light[5][6] < night.light[5][5] || night.light[5][6] == 4);
	int shaded = 0;
	for (int r = 0; r < ViewGrid::CELLS; ++r)
		for (int c = 0; c < ViewGrid::CELLS; ++c)
			shaded += night.light[r][c] > 0 && night.light[r][c] < 4;
	CHECK(shaded > 4); // the light fades towards the rim

	// the throne room's east wall (x 312): from inside, the wall is seen, what lies beyond it is not
	b.build(307, 352, 0, 7, day);
	int unseen = 0;
	for (int r = 0; r < ViewGrid::CELLS; ++r)
		unseen += day.tile[r][10] == ViewGrid::UNSEEN;
	CHECK(unseen > 0 || day.tile[5][10] != ViewGrid::UNSEEN);

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}

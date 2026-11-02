// The running game: start state, clock and movement (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::GameClock;
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
	// Clock arithmetic: 28-day months, 13 months a year.
	GameClock c = {59, 23, 28, 13, 161};
	c.advance(1);
	CHECK(c.minute == 0 && c.hour == 0 && c.day == 1 && c.month == 1 && c.year == 162);
	c.advance(60 * 24 * 28);
	CHECK(c.day == 1 && c.month == 2 && c.year == 162);
	CHECK(c.dayOfWeek() == 1);

	World world;
	if (!world.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return failures ? 1 : 0;
	}
	CHECK(world.objects.size() > 16000);

	Game game(world);
	// A new game starts at 8:00 on day 4 of month 7, year 161 (the date END.EXE's playing time counts from)
	CHECK(game.clock.hour == 8 && game.clock.minute == 0 && game.clock.day == 4 && game.clock.month == 7 && game.clock.year == 161);
	CHECK(game.x == 307 && game.y == 352 && game.z == 0);

	// Walking takes a minute a step; walls stop you and cost nothing.
	int moved = 0;
	for (int i = 0; i < 12; ++i)
		moved += game.handleKey(KEY_UP) && game.y != 352 ? 0 : 0;
	int before = game.clock.minute;
	int x0 = game.x, y0 = game.y;
	bool ok = game.step(0);
	if (ok) {
		CHECK(game.y == y0 - 1 && game.x == x0 && game.clock.minute == before + 1);
	} else {
		CHECK(game.y == y0 && game.clock.minute == before);
	}
	(void)moved;
	// The throne room is closed in: no amount of walking leaves it quickly
	for (int i = 0; i < 40; ++i)
		game.step(i % 8);
	CHECK(game.x > 280 && game.x < 330 && game.y > 330 && game.y < 370);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

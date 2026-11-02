// The running game: start state, clock and movement (skipped when the game data is absent).

#include "ultima6/gypsy.h"
#include "ultima6/session.h"

#include <string>
#include <vector>

#include <cstdio>
#include <string>
#include <vector>

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

	// Talking and looking go through the same key handler as the player's keys.
	{
		World w2;
		CHECK(w2.load("u6/game"));
		Game g2(w2);
		auto text = [&]() {
			std::string all;
			for (const std::string &l : g2.scroll.lines())
				all += l + " ";
			return all;
		};
		g2.handleKey(KEY_DOWN); // to 307,353, with a companion to the east
		const int DIRS[4] = {KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP};
		for (int d = 0; d < 4 && text().find("You see") == std::string::npos; ++d) {
			g2.handleKey('t');
			g2.handleKey(DIRS[d]);
		}
		CHECK(text().find("You see") != std::string::npos);
		for (int i = 0; i < 40; ++i)
			g2.handleKey(KEY_ESC); // keys at page breaks and prompts must never hang or crash
		for (int i = 0; i < 4; ++i) {
			g2.handleKey('b');
			g2.handleKey('y');
			g2.handleKey('e');
			g2.handleKey(KEY_ENTER);
			g2.handleKey(KEY_SPACE);
		}
		g2.handleKey('l');
		g2.handleKey(KEY_ENTER);

		// Time passes and people go about their schedules.
		std::vector<std::pair<int, int>> before;
		for (int a = 8; a < 0xE0; ++a) {
			int k = w2.actorObject(a);
			before.push_back(k >= 0 ? std::make_pair((int)w2.objects[k].x, (int)w2.objects[k].y) : std::make_pair(-1, -1));
		}
		for (int i = 0; i < 12 * 60; ++i)
			g2.handleKey(KEY_SPACE);
		int moved = 0;
		for (int a = 8; a < 0xE0; ++a) {
			int k = w2.actorObject(a);
			if (k >= 0 && before[a - 8] != std::make_pair((int)w2.objects[k].x, (int)w2.objects[k].y))
				++moved;
		}
		std::printf("%d actors moved in twelve hours\n", moved);
		CHECK(moved > 10);
	}

	// A new game: the world built from LZOBJBLK and LZDNGBLK with a freshly made character.
	{
		::Ultima6::Ultima6::NewCharacter c;
		c.name = "Tester";
		c.female = false;
		c.portrait = 2;
		c.stats.strength = 17;
		c.stats.dexterity = 16;
		c.stats.intelligence = 18;
		c.stats.karma = 75;
		World w4;
		CHECK(w4.load("u6/game", &c));
		CHECK(std::string((const char *)w4.actors.partyNames[0]) == "Tester");
		CHECK(w4.actors.strength[1] == 17);
		CHECK(w4.objects.size() > 10000);
		Game g4(w4);
		CHECK(g4.x > 0 && g4.y > 0);
		Bytes px;
		CHECK(w4.portrait(1, px)); // the portrait chosen at creation
	}

	// Using a door from the west opens it, using it again closes it.
	{
		World w3;
		CHECK(w3.load("u6/game"));
		Game g3(w3);
		int door = -1;
		for (size_t i = 0; i < w3.objects.size() && door < 0; ++i) {
			const World::Obj &o = w3.objects[i];
			if (o.type >= 297 && o.type <= 300 && o.onMap() && o.z == 0 && (o.frame == 5 || o.frame == 7))
				door = (int)i;
		}
		CHECK(door >= 0);
		if (door >= 0) {
			int frame = w3.objects[door].frame;
			int px = w3.objects[door].x - 1, py = w3.objects[door].y;
			w3.moveObject(w3.actorObject(1), px, py, 0);
			g3.x = px;
			g3.y = py;
			g3.z = 0;
			g3.handleKey('u');
			g3.handleKey(KEY_RIGHT);
			CHECK(w3.objects[door].frame == frame - 4);
			g3.handleKey('u');
			g3.handleKey(KEY_RIGHT);
			CHECK(w3.objects[door].frame == frame);
		}
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

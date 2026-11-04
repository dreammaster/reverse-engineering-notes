// Equipment: the places things are worn (sub_1D766) and cmd_ready's rules (skipped when the game data is absent).

#include "ultima6/equipment.h"
#include "ultima6/session.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace Ultima6;
using ::Ultima6::Ultima6::Equipment;
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

static std::string text(const Game &g) {
	std::string all;
	for (const std::string &l : g.log)
		all += l;
	return all;
}

int main() {
	CHECK(Equipment::slotClass(0x200) == 0, "helmet");
	CHECK(Equipment::slotClass(0x219) == 1, "neck");
	CHECK(Equipment::slotClass(0x210) == 4, "body");
	CHECK(Equipment::slotClass(0x21A) == 7, "feet");
	CHECK(Equipment::slotClass(0x228) == 8, "two-handed");
	CHECK(Equipment::slotClass(0x208) == 5, "shield");
	CHECK(Equipment::slotClass(0x220) == 2, "one-handed");
	CHECK(Equipment::slotClass(0x258) == 9, "arm");
	CHECK(Equipment::slotClass(0x100) == -1, "not wearable");

	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return failures ? 1 : 0;
	}
	Game g(w);
	std::vector<int> inv;
	w.inventory(1, inv, true);
	for (int i : inv)
		w.deleteObject(i);
	// the types whose first tile is in a class
	auto typeOfTile = [&](int tile) {
		for (int t = 0; t < 0x200; ++t)
			if (w.baseTile.firstTile[t] == tile)
				return t;
		return -1;
	};
	int helmet = typeOfTile(0x200);
	CHECK(helmet >= 0, "a helmet type");
	if (helmet >= 0) {
		w.addToInventory(1, helmet, 0, 1);
		w.objects[w.addToInventory(1, helmet, 0, 1)].quantity = 1;
		g.log.clear();
		g.handleKey('i');
		g.handleKey(KEY_ENTER);
		CHECK(text(g).find("Ready-") != std::string::npos, "Ready-");
		Equipment::Slots s = Equipment::of(w, 1);
		CHECK(s.object[0] >= 0, "the helmet is on the head");
		g.log.clear();
		g.handleKey(KEY_DOWN);
		g.handleKey(KEY_ENTER);
		CHECK(text(g).find("No place to put!") != std::string::npos, "a second helmet has no place");
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

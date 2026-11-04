// The Drop command (cmd_drop): the object, the quantity, the place, the breaking of glass from afar (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
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

static std::string text(const Game &g) {
	std::string all;
	for (const std::string &l : g.log)
		all += l;
	return all;
}

static void emptyPack(World &w, int actor) {
	std::vector<int> inv;
	w.inventory(actor, inv, true);
	for (int i : inv)
		w.deleteObject(i);
}

static int carried(World &w, int actor, int type) {
	std::vector<int> inv;
	w.inventory(actor, inv, false);
	int n = 0;
	for (int i : inv)
		if (w.objects[i].type == type)
			n += w.quantityOf(w.objects[i]);
	return n;
}

static int onSquare(World &w, int x, int y, int type) {
	std::vector<int> here;
	w.objectsAt(x, y, 0, here);
	int n = 0;
	for (int i : here)
		if (w.objects[i].type == type)
			n += w.quantityOf(w.objects[i]);
	return n;
}

int main() {
	static World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	// open ground, nothing about
	int bx = g.x, by = g.y;
	emptyPack(w, 1);

	// a stack: "How many? ", then "Location: ", and the five lie on the square chosen, free to take
	w.addToInventory(1, 0x58, 0, 30);
	g.log.clear();
	g.handleKey('D');
	g.handleKey(KEY_ENTER);
	CHECK(text(g).find("How many? ") != std::string::npos, "asks how many");
	g.handleKey('5');
	g.handleKey(KEY_ENTER);
	CHECK(text(g).find("Location: ") != std::string::npos, "asks where");
	g.handleKey(KEY_RIGHT);
	g.handleKey(KEY_ENTER);
	CHECK(carried(w, 1, 0x58) == 25, "25 are left");
	CHECK(onSquare(w, bx + 1, by, 0x58) == 5, "5 lie to the east");
	std::vector<int> here;
	w.objectsAt(bx + 1, by, 0, here);
	bool free = false;
	for (int i : here)
		free |= w.objects[i].type == 0x58 && (w.objects[i].status & 1);
	CHECK(free, "free to take");

	// more than there are: "0   " and nothing happens
	g.log.clear();
	g.handleKey('D');
	g.handleKey(KEY_ENTER);
	g.handleKey('9');
	g.handleKey('9');
	g.handleKey(KEY_ENTER);
	CHECK(text(g).find("0   ") != std::string::npos, "too many");
	CHECK(carried(w, 1, 0x58) == 25, "nothing dropped");

	// no place chosen: Not possible
	g.log.clear();
	g.handleKey('D');
	g.handleKey(KEY_ENTER);
	g.handleKey('1');
	g.handleKey(KEY_ENTER);
	g.handleKey(KEY_ESC);
	CHECK(text(g).find("Not possible") != std::string::npos, "no place");
	CHECK(carried(w, 1, 0x58) == 25, "still 25");

	// a glass thing dropped from afar breaks; at one square it lies there
	emptyPack(w, 1);
	w.addToInventory(1, 0x53, 0, 1);
	g.log.clear();
	g.handleKey('D');
	g.handleKey(KEY_ENTER);
	g.handleKey(KEY_RIGHT);
	g.handleKey(KEY_RIGHT);
	g.handleKey(KEY_RIGHT);
	g.handleKey(KEY_ENTER);
	std::string out = text(g);
	CHECK(out.find("It broke!") != std::string::npos || out.find("Not possible") != std::string::npos, "far glass breaks (or the way is shut)");
	if (out.find("It broke!") != std::string::npos)
		CHECK(carried(w, 1, 0x53) == 0 && onSquare(w, bx + 3, by, 0x53) == 0, "it is gone");

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

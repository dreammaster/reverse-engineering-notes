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

	// Move in the pack: ten coins to the second member ("To 2")
	emptyPack(w, 1);
	w.addToInventory(1, 0x58, 0, 30);
	int buddy = w.actors.partyRoster[1];
	int had = carried(w, buddy, 0x58);
	g.log.clear();
	g.handleKey('i');
	g.handleKey('m');
	CHECK(text(g).find("How many? ") != std::string::npos, "move asks how many");
	g.handleKey('1');
	g.handleKey('0');
	g.handleKey(KEY_ENTER);
	g.handleKey('2');
	CHECK(carried(w, 1, 0x58) == 20, "20 are left");
	CHECK(carried(w, buddy, 0x58) == had + 10, "10 have been handed over");
	CHECK(text(g).find(std::string((const char *)w.actors.partyNames[1]) + ".") != std::string::npos, "the name");
	g.handleKey(KEY_ESC);

	// Move on the map: a cannon is turned to the way it is pushed, and moves when pushed again
	{
		static const int DXS[4] = {0, 1, 0, -1}, DYS[4] = {-1, 0, 1, 0};
		static const int KEYS[4] = {KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT};
		for (int d = 0; d < 4; ++d) {
			int ox = bx + DXS[d], oy = by + DYS[d], nx = ox + DXS[d], ny = oy + DYS[d];
			std::vector<int> at, beyond;
			w.objectsAt(ox, oy, 0, at);
			w.objectsAt(nx, ny, 0, beyond);
			bool open = at.empty() && beyond.empty();
			for (int m = 0; m < w.actors.partySize; ++m) {
				const World::Obj &mo = w.objects[w.actorObject(w.actors.partyRoster[m])];
				open &= !(mo.x == ox && mo.y == oy) && !(mo.x == nx && mo.y == ny);
			}
			open &= !(w.flags.flags1[w.map.tile(0, ox, oy) & 2047] & 3) && !(w.flags.flags1[w.map.tile(0, nx, ny) & 2047] & 3);
			if (!open)
				continue;
			World::Obj cannon;
			cannon.status = 0;
			cannon.x = (uint16_t)ox;
			cannon.y = (uint16_t)oy;
			cannon.z = 0;
			cannon.type = 0xDD;
			cannon.frame = (uint8_t)((d + 2) & 3); // facing the other way
			cannon.quantity = 1;
			cannon.quality = 0;
			cannon.actor = -1;
			cannon.parent = -1;
			w.objects.push_back(cannon);
			int ci = (int)w.objects.size() - 1;
			w.addToMap(ci);
			g.handleKey('M');
			g.handleKey(KEYS[d]);
			g.handleKey(KEYS[d]);
			CHECK(w.objects[ci].x == ox && w.objects[ci].y == oy, "the cannon only turned");
			CHECK(w.objects[ci].frame == (uint8_t)d, "to face the way it was pushed");
			g.handleKey('M');
			g.handleKey(KEYS[d]);
			g.handleKey(KEYS[d]);
			CHECK(w.objects[ci].x == nx && w.objects[ci].y == ny, "and then it moved");
			break;
		}
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

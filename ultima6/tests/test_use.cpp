// The Use command's handlers: doors, chests, food, candles (skipped when the game data is absent).

#include "ultima6/session.h"

#include <cstdio>
#include <string>
#include <vector>

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

static int put(World &w, int type, int frame, int x, int y, int z) {
	World::Obj o;
	o.status = 0;
	o.x = (uint16_t)x;
	o.y = (uint16_t)y;
	o.z = (uint8_t)z;
	o.type = (uint16_t)type;
	o.frame = (uint8_t)frame;
	o.quantity = 1;
	o.quality = 0;
	o.actor = -1;
	o.parent = -1;
	w.objects.push_back(o);
	w.addToMap((int)w.objects.size() - 1);
	return (int)w.objects.size() - 1;
}

// Uses what lies east of a spot in the open.
static void useEast(World &w, Game &g, int px, int py) {
	w.moveObject(w.actorObject(1), px, py, 0);
	g.x = px;
	g.y = py;
	g.z = 0;
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
}

int main() {
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	int px = g.x, py = g.y;

	// a chest: closed (frame 1) it opens and its contents lie on the ground; open (frame 0) it closes
	{
		int chest = put(w, 98, 1, px + 1, py, 0);
		int gold = put(w, 0x58, 0, 0, 0, 0);
		w.objects[gold].status = (uint8_t)ObjRecord::IN_CONTAINER;
		w.objects[gold].parent = chest;
		w.objects[gold].quantity = 5;
		w.addToMap(gold); // no effect for a contained object
		useEast(w, g, px, py);
		CHECK(w.objects[chest].frame == 0);
		CHECK(logText(g).find("opened!") != std::string::npos);
		CHECK(logText(g).find("Searching here, you find") != std::string::npos);
		CHECK(w.objects[gold].onMap() && w.objects[gold].x == px + 1 && w.objects[gold].y == py);
		useEast(w, g, px, py);
		CHECK(w.objects[chest].frame == 1);
		CHECK(logText(g).find("closed!") != std::string::npos);
	}

	// a locked door stays locked and says so; a magically locked one says so
	{
		w.objects.reserve(w.objects.size() + 16);
		int door = put(w, 297, 9, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 9);
		CHECK(logText(g).find("locked\n") != std::string::npos);
		w.objects[door].frame = 13;
		useEast(w, g, px, py);
		CHECK(w.objects[door].frame == 13);
		CHECK(logText(g).find("magically locked!") != std::string::npos);
	}

	// food is eaten, a candle is lit and doused
	{
		int food = put(w, 0x5F, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[food].deleted);
		CHECK(logText(g).find("You eat the food.") != std::string::npos);
		int candle = put(w, 0x7A, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[candle].frame == 1);
		CHECK(logText(g).find("candle is lit.") != std::string::npos);
		useEast(w, g, px, py);
		CHECK(w.objects[candle].frame == 0);
		CHECK(logText(g).find("candle is doused.") != std::string::npos);
	}

	// a well fills an empty bucket
	{
		put(w, 0xE9, 0, px + 1, py, 0);
		w.addToInventory(1, 0xB2, 0, 1);
		useEast(w, g, px, py);
		CHECK(logText(g).find("Done") != std::string::npos);
		std::vector<int> inv;
		w.inventory(1, inv);
		bool water = false;
		for (int i : inv)
			water |= w.objects[i].type == 0xB3 && !w.objects[i].deleted;
		CHECK(water);
	}

	std::printf("%s\n", failures ? "FAILED" : "ok");
	return failures ? 1 : 0;
}

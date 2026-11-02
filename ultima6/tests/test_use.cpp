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

	// looking at a sign reads BOOK.DAT: quality 4 is "The bones of Zog."
	{
		int sign = put(w, 0x98, 0, px + 1, py, 0);
		w.objects[sign].quality = 4;
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find(":\n\nThe bones of Zog.") != std::string::npos);
	}

	// Move: a key to the east is pushed one square on (or is "Blocked")
	{
		int key = put(w, 0x40, 0, px + 1, py, 0);
		w.moveObject(w.actorObject(1), px, py, 0);
		g.x = px;
		g.y = py;
		g.handleKey('m');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_RIGHT);
		bool moved = w.objects[key].x == px + 2 && w.objects[key].y == py;
		bool stayed = w.objects[key].x == px + 1 && logText(g).find("Blocked") != std::string::npos;
		CHECK(moved || stayed);
		CHECK(logText(g).find("East.") != std::string::npos);
	}

	// a blue moon gate at the start (day 4, 8:00): the moons' phases are 4 and 6, so the party goes to the gate of the second moon, gate 6 at 23,22 (level 1)
	{
		World w2;
		w2.load("u6/game");
		Game g2(w2);
		int gx = g2.x, gy = g2.y;
		put(w2, 0x55, 0, gx, gy - 1, 0);
		CHECK(g2.step(0));
		CHECK(g2.x == 23 && g2.y == 22 && g2.z == 1);
	}

	// Create Food (circle 1, "imy"): 1-10 food for the caster
	{
		World w3;
		w3.load("u6/game");
		Game g3(w3);
		int book = w3.addToInventory(1, 57, 0, 1);
		w3.objects[book].status = (uint8_t)((w3.objects[book].status & ~0x18) | 0x18);
		World::Obj spell = w3.objects[book];
		spell.type = 58;
		spell.quality = 255;
		spell.status = 0x08;
		spell.parent = book;
		spell.actor = -1;
		w3.objects.push_back(spell);
		w3.addToInventory(1, 69, 0, 5);
		w3.addToInventory(1, 67, 0, 5);
		w3.addToInventory(1, 68, 0, 5);
		g3.handleKey('c');
		g3.handleKey('1');
		g3.handleKey('i');
		g3.handleKey('m');
		g3.handleKey('y');
		g3.handleKey(KEY_ENTER);
		std::string text;
		for (const std::string &l : g3.log)
			text += l;
		CHECK(text.find("Create ") != std::string::npos && text.find(" food.") != std::string::npos);
		std::vector<int> inv;
		w3.inventory(1, inv);
		bool food = false;
		for (int i : inv)
			food |= w3.objects[i].type == 0x81 && !w3.objects[i].deleted;
		CHECK(food);
	}

	// burying a moonstone on soft ground sets the place of its gate and, the moons being up, a blue moon gate appears there
	{
		World w4;
		w4.load("u6/game");
		Game g4(w4);
		int bx = -1, by = -1;
		for (int yy = g4.y - 20; yy < g4.y + 20 && bx < 0; ++yy)
			for (int xx = g4.x - 20; xx < g4.x + 20 && bx < 0; ++xx) {
				int t = w4.map.tile(0, xx, yy);
				if (t >= 0x10 && t <= 0x6F)
					bx = xx, by = yy;
			}
		CHECK(bx >= 0);
		if (bx >= 0) {
			w4.moveObject(w4.actorObject(1), bx, by, 0);
			g4.x = bx;
			g4.y = by;
			g4.z = 0;
			int stone = put(w4, 0x49, 3, bx, by, 0);
			(void)stone;
			g4.handleKey('u');
			g4.handleKey(KEY_ENTER); // the own square
			std::string text;
			for (const std::string &l : g4.log)
				text += l;
			CHECK(text.find("buried.") != std::string::npos);
			const std::vector<uint8_t> &gl = w4.actors.globals;
			CHECK((gl[0x2A + 18] | (gl[0x2A + 19] << 8)) == bx && (gl[0x2A + 20] | (gl[0x2A + 21] << 8)) == by);
			std::vector<int> here;
			w4.objectsAt(bx, by, 0, here);
			bool gate = false;
			for (int i : here)
				gate |= w4.objects[i].type == 0x55;
			CHECK(gate);
		}
	}

	std::printf("%s\n", failures ? "FAILED" : "ok");
	return failures ? 1 : 0;
}

// Martian Dreams' Use (the recipes of seg059 / seg060): an object without a recipe "can't be used", a door of recipe 44 opens and shuts, a locked one says so (skipped without data).

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

static void useEast(World &w, Game &g, int px, int py) {
	w.moveObject(w.actorObject(1), px, py, 0);
	g.x = px;
	g.y = py;
	g.z = 0;
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
}

int main() {
	static World w;
	w.game = GameId::MartianDreams;
	if (!w.load("md/game/MARTIAN")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 32);
	Game g(w);
	int px = g.x, py = g.y;

	for (int i = 0; i < w.actors.partySize; ++i)
		w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);
	std::vector<int> there;
	w.objectsAt(px + 1, py, 0, there);
	for (int i : there)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
	// a door (type 0xDE takes recipe 44): frame bit 2 turns over
	int door = put(w, 0xDE, 0, px + 1, py, 0);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 2);
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	// locked: quality bit 0x80
	w.objects[door].quality = 0x80;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[door].frame == 0);
	CHECK(logText(g).find("locked") != std::string::npos);
	w.deleteObject(door);

	// an unlit oil lamp (0x73) with a match: no fuel; with a can of lamp oil (0x7C) it takes fuel and lights (type 0x74), the match is used up
	{
		w.deleteObject(door);
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int lamp = put(w, 0x73, 0, px + 1, py, 0);
		w.addToInventory(w.actors.partyRoster[0], 0x6B, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("There is no fuel") != std::string::npos);
		CHECK(w.objects[lamp].type == 0x73);
		w.addToInventory(w.actors.partyRoster[0], 0x7C, 0, 1);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[lamp].type == 0x74 && w.objects[lamp].quality == 0xFF);
		w.deleteObject(lamp);
	}

	// a thing with no recipe (type 0x1FE has none)
	int thing = put(w, 0x1FE, 0, px + 1, py, 0);
	(void)thing;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("can't be used") != std::string::npos || logText(g).find("nothing") != std::string::npos);

	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}

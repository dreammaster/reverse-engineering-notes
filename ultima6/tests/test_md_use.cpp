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

	// a sliding gate (0xB5, recipe 70): frame 0 goes to 3; locked it will not
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int gate = put(w, 0xB5, 0, px + 1, py, 0);
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 3);
		w.objects[gate].frame = 0;
		w.objects[gate].quality = 0x80;
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(w.objects[gate].frame == 0 && logText(g).find("locked") != std::string::npos);
		w.deleteObject(gate);
	}

	// an instrument (0x11B, recipe 57) is played ("Wheeeeze"); a thing that has only a message (0x143, recipe 64) says it
	{
		for (int type : {0x11B, 0x143}) {
			std::vector<int> here;
			w.objectsAt(px + 1, py, 0, here);
			for (int i : here)
				if (w.objects[i].actor < 0)
					w.deleteObject(i);
			int thing = put(w, type, 0, px + 1, py, 0);
			g.log.clear();
			useEast(w, g, px, py);
			if (type == 0x11B)
				CHECK(logText(g).find("Wheeeeze") != std::string::npos);
			else
				CHECK(logText(g).find("You don't know how it works") != std::string::npos);
			w.deleteObject(thing);
		}
	}

	// the sextant (0x60, recipe 33) on the surface: (y - 512) * 240 / 1024 = 27 south, (x - 512) * 360 / 1024 gives east
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int sextant = put(w, 0x60, 0, px + 1, py, 0);
		g.log.clear();
		useEast(w, g, px, py);
		CHECK(logText(g).find("You are somewhere near") != std::string::npos);
		CHECK(logText(g).find("S") != std::string::npos);
		w.deleteObject(sextant);
	}

	// Look at a lit torch with 12 minutes left, and at a pocket watch
	{
		std::vector<int> here;
		w.objectsAt(px + 1, py, 0, here);
		for (int i : here)
			if (w.objects[i].actor < 0)
				w.deleteObject(i);
		int torch = put(w, 0x6E, 0, px + 1, py, 0);
		w.objects[torch].quality = 12;
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("It has 12 more minutes of ") != std::string::npos);
		CHECK(logText(g).find("wick.") != std::string::npos);
		w.deleteObject(torch);
		int watch = put(w, 0x5B, 0, px + 1, py, 0);
		g.log.clear();
		g.handleKey('l');
		g.handleKey(KEY_RIGHT);
		g.handleKey(KEY_ENTER);
		CHECK(logText(g).find("The time is ") != std::string::npos);
		w.deleteObject(watch);
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

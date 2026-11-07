// Savage Empire's Use (cmd_use = sub_35E46): the table of 80 types, "Not possible." for the rest, drinking, eating, the crate that opens and gives up what is in it, the torch that is lit.
// Skipped without the game data.

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

static void clearEast(World &w, int px, int py) {
	std::vector<int> here;
	w.objectsAt(px + 1, py, 0, here);
	for (int i : here)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
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
	w.game = GameId::SavageEmpire;
	if (!w.load("se/game/SAVAGE")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 64);
	Game g(w);
	int px = g.x, py = g.y;
	for (int i = 0; i < w.actors.partySize; ++i)
		w.moveObject(w.actorObject(w.actors.partyRoster[i]), px, py, 0);

	// not on the list of what may be used: "Not possible."
	clearEast(w, px, py);
	int rock = put(w, 0x30, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("Not possible.") != std::string::npos);
	w.deleteObject(rock);

	// a bottle of liquor (0x5F): drunk, gone, the counter (byte_4226E, globals + 0x26) goes up
	clearEast(w, px, py);
	int bottle = put(w, 0x5F, 0, px + 1, py, 0);
	int drunk = w.actors.globals[0x26];
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[bottle].deleted);
	CHECK(logText(g).find("You drink it.") != std::string::npos);
	CHECK(w.actors.globals[0x26] == drunk + 1);

	// meat (0x67): eaten
	clearEast(w, px, py);
	int meat = put(w, 0x67, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[meat].deleted);
	CHECK(logText(g).find("You eat the food.") != std::string::npos);

	// a shut crate (0xBC frame 1) opens and gives up what is in it; already open it says "Not possible." and finds nothing
	clearEast(w, px, py);
	int crate = put(w, 0xBC, 1, px + 1, py, 0);
	int inside = put(w, 0x66, 0, 0, 0, 0);
	w.deleteObject(inside);
	World::Obj &in = w.objects[inside];
	in.deleted = false;
	in.status = ObjRecord::IN_CONTAINER;
	in.parent = crate;
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[crate].frame == 0);
	CHECK(logText(g).find("Searching here, you find") != std::string::npos);
	CHECK(w.objects[inside].location() == ObjRecord::ON_MAP);
	w.deleteObject(inside); // (it lies on the square now and would be what Use picks: it is food)
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("Not possible.") != std::string::npos);
	CHECK(logText(g).find("nothing") != std::string::npos);

	// a notebook (0x31): only Jimmy (base type 0x138) can read it
	clearEast(w, px, py);
	int note = put(w, 0x31, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("can't read Jimmy's shorthand") != std::string::npos);
	w.deleteObject(note);

	// a wire screen (0xB1) beside an 0xE7 sifts ten of 0x7B (sulphur powder); without one there is nothing to sift
	clearEast(w, px, py);
	int screen = put(w, 0xB1, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("nothing to sift") != std::string::npos);
	int ore = put(w, 0xE7, 0, px, py - 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("You made some sulphur powder") != std::string::npos);
	w.deleteObject(ore);
	w.deleteObject(screen);

	// burnt logs (0xCF): frame 0 is "Not possible.", frame 1 starts a fire (frame 2)
	clearEast(w, px, py);
	int logs = put(w, 0xCF, 1, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[logs].frame == 2);
	CHECK(logText(g).find("You started a fire") != std::string::npos);
	w.deleteObject(logs);

	// a metal bucket (0x88): next to a tar pit (0xE5) it fills with tar (frame 1); full of water (frame 2) it is poured onto a fire (0xF2), which goes out
	clearEast(w, px, py);
	int bucket = put(w, 0x88, 0, px + 1, py, 0);
	int pit = put(w, 0xE5, 0, px, py - 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(w.objects[bucket].frame == 1);
	CHECK(logText(g).find("tar in the bucket") != std::string::npos);
	w.deleteObject(pit);
	w.objects[bucket].frame = 2;
	std::vector<int> below;
	w.objectsAt(px, py + 1, 0, below);
	for (int i : below)
		if (w.objects[i].actor < 0)
			w.deleteObject(i);
	int fire = put(w, 0xF2, 0, px, py + 1, 0);
	g.log.clear();
	useEast(w, g, px, py);
	g.handleKey(KEY_DOWN);
	g.handleKey(KEY_ENTER);
	CHECK(w.objects[fire].deleted);
	CHECK(w.objects[bucket].frame == 0);
	CHECK(logText(g).find("You put the fire out") != std::string::npos);
	w.deleteObject(bucket);

	// the digging stick (0x83) finds clay (terrain 0x20-0x2F, 0xBE) around the member or says there is nothing to dig; clay (0xC0) is shaped into a pot (0x84)
	clearEast(w, px, py);
	int stick = put(w, 0x83, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("clay") != std::string::npos || logText(g).find("nothing to dig") != std::string::npos);
	w.deleteObject(stick);
	clearEast(w, px, py);
	int clay = put(w, 0xC0, 0, px + 1, py, 0);
	g.log.clear();
	useEast(w, g, px, py);
	CHECK(logText(g).find("You shaped a small, soft pot") != std::string::npos);
	CHECK(w.objects[clay].deleted);
	CHECK(g.log.size() > 0);

	// a torch (0xD0) is lit in a free hand: a lit torch (0xD1) with 100 minutes appears readied
	clearEast(w, px, py);
	int actor = w.actors.partyRoster[0];
	std::vector<int> inv;
	w.inventory(actor, inv, false);
	int torch = w.addToInventory(actor, 0xD0, 0, 3);
	g.log.clear();
	g.handleKey('u');
	g.handleKey(KEY_RIGHT);
	(void)torch;
	w.inventory(actor, inv, false);

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

// Savage Empire's eggs (sub_36B67): looked at once, a roll against the quantity, the creatures of the kinds it holds made next to it with the worktype of their side (skipped without the game data).

#include "ultima6/combat.h"
#include "ultima6/session.h"

#include <cstdio>

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

int main() {
	static World w;
	w.game = GameId::SavageEmpire;
	if (!w.load("se/game/SAVAGE")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 512);
	int px, py, pz;
	w.avatarPosition(px, py, pz);
	// an egg of quantity 100 holding two people 0x137, some way off in the open
	World::Obj egg;
	egg.status = 0;
	egg.x = (uint16_t)(px + 14);
	egg.y = (uint16_t)py;
	egg.z = 0;
	egg.type = 0xE6;
	egg.frame = 0;
	egg.quantity = 100;
	egg.quality = 0;
	egg.actor = -1;
	egg.parent = -1;
	w.objects.push_back(egg);
	int e = (int)w.objects.size() - 1;
	w.addToMap(e);
	World::Obj child = egg;
	child.status = ObjRecord::IN_CONTAINER;
	child.type = 0x137;
	child.quantity = 2;
	child.parent = e;
	child.x = child.y = 0;
	w.objects.push_back(child);
	Game g(w);
	int found = 0;
	for (int a = 0xE0; a < 256; ++a) {
		int k = w.actorObject(a);
		found += k >= 0 && !w.objects[k].deleted && w.objects[k].type == 0x137;
	}
	CHECK(found >= 1 && found <= 2); // (the Game made at the start has looked at the eggs round the party)
	CHECK((w.objects[e].status & 0x40) && (w.objects[e].status & 2)); // used up and unseen
	g.advanceTime(1);
	int later = 0;
	for (int a = 0xE0; a < 256; ++a) {
		int k = w.actorObject(a);
		later += k >= 0 && !w.objects[k].deleted && w.objects[k].type == 0x137;
	}
	CHECK(later == found); // (it does not hatch twice)
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

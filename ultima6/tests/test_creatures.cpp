// Eggs hatching and the creatures' turns on the real world (skipped when the game data is absent).

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
	World w;
	if (!w.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Game g(w);
	// an egg on the surface holding creatures that always hatches
	int egg = -1, embryoType = 0;
	for (size_t i = 0; i < w.objects.size() && egg < 0; ++i) {
		const World::Obj &o = w.objects[i];
		if (o.type != 335 || !o.onMap() || o.z != 0 || o.quality >= 10 || o.quantity != 100)
			continue;
		for (const World::Obj &c : w.objects)
			if (c.location() == 0x08 && c.parent == (int)i && c.type >= 342 && c.type != 413 && ::Ultima6::Ultima6::Combat::stats(c.type)) {
				egg = (int)i;
				embryoType = c.type;
				break;
			}
	}
	CHECK(egg >= 0);
	if (egg >= 0) {
		int ex = w.objects[egg].x, ey = w.objects[egg].y;
		w.moveObject(w.actorObject(1), ex + 15, ey, 0);
		g.x = ex + 15;
		g.y = ey;
		g.z = 0;
		g.advanceTime(1);
		int found = 0;
		for (int a = 0xE0; a < 256; ++a) {
			int k = w.actorObject(a);
			if (k >= 0 && !w.objects[k].deleted && w.objects[k].type == embryoType)
				++found;
		}
		std::printf("egg at %d,%d hatched %d creatures of type %d\n", ex, ey, found, embryoType);
		CHECK(found > 0);
		// walk far away: they are gone
		w.moveObject(w.actorObject(1), ex + 200, ey, 0);
		g.x = ex + 200;
		g.advanceTime(1);
		int left = 0;
		for (int a = 0xE0; a < 256; ++a) {
			int k = w.actorObject(a);
			left += k >= 0 && !w.objects[k].deleted && w.objects[k].type == embryoType && w.objects[k].x < ex + 100;
		}
		CHECK(left == 0);
	}
	// the combat formation: the companions take their places on the line towards the enemy and strike it (wt_combat_front 2E454 / rear 2E4B9, sub_2A4B6)
	if (w.actors.partySize >= 3) {
		int egg2 = -1;
		for (size_t i = 0; i < w.objects.size() && egg2 < 0; ++i) {
			const World::Obj &o = w.objects[i];
			if (o.type == 335 && o.onMap() && o.z == 0 && o.quality < 10 && o.quantity == 100)
				egg2 = (int)i;
		}
		int ex = w.objects[egg2].x, ey = w.objects[egg2].y;
		w.moveObject(w.actorObject(1), ex + 7, ey, 0);
		g.x = ex + 7;
		g.y = ey;
		g.z = 0;
		g.advanceTime(1);
		int foe = -1;
		for (int a = 0xE0; a < 256 && foe < 0; ++a) {
			int k = w.actorObject(a);
			if (k >= 0 && !w.objects[k].deleted)
				foe = a;
		}
		CHECK(foe >= 0);
		if (foe >= 0) {
			w.actors.worktype[foe] = 3;
			w.actors.status[foe] |= 0x20;
			w.actors.hp[foe] = 250;
			w.moveObject(w.actorObject(foe), g.x + 5, g.y, 0);
			for (int m = 1; m < w.actors.partySize; ++m)
				w.moveObject(w.actorObject(w.actors.partyRoster[m]), g.x - m, g.y, 0);
			g.handleKey('X');
			for (int r = 0; r < 40; ++r)
				g.handleKey(' ');
			int bx = w.objects[w.actorObject(w.actors.partyRoster[1])].x;
			std::printf("front companion at %d (leader %d), foe hp %d\n", bx, g.x, w.actors.hp[foe]);
			CHECK(bx > g.x - 1);          // the companion came up to the front of the line
			CHECK(w.actors.hp[foe] < 250); // and the foe has been struck
		}
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

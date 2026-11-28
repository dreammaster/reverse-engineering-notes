// Savage Empire's blows (Combat::seAttack: attack_with_weapon, sub_29B5A): out of range, a web or tree the weapon does nothing to, ammunition used up, the spear that lands, the thrown weapon
// that is replaced from the pack, the grenade that goes off where it lands, a blow at a thing (skipped without the game data).

#include "ultima6/combat.h"
#include "ultima6/session.h"

#include <cstdio>
#include <cstdlib>

using namespace Ultima6;
using ::Ultima6::Ultima6::Combat;
using ::Ultima6::Ultima6::TextWindow;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

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

static int ready(World &w, int actor, int type, int quantity = 1) {
	int i = w.addToInventory(actor, type, 0, quantity);
	w.objects[i].status = (uint8_t)((w.objects[i].status & ~ObjRecord::LOCATION_MASK) | ObjRecord::READIED);
	return i;
}

static int count(World &w, int type, int x, int y) {
	std::vector<int> here;
	w.objectsAt(x, y, 0, here);
	int n = 0;
	for (int i : here)
		n += !w.objects[i].deleted && w.objects[i].type == type;
	return n;
}

int main() {
	static World w;
	w.game = GameId::SavageEmpire;
	if (!w.load("se/game/SAVAGE")) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	w.objects.reserve(w.objects.size() + 256);
	BorlandRandom rng(3);
	TextWindow scroll(22, 14, 17, 10);
	Combat c(w, rng, scroll);
	int explodeX = -1, explodeY = -1, burnX = -1, hurtThing = -1;
	bool device = false;
	c.onExplode = [&](int x, int y, bool d) {
		explodeX = x;
		explodeY = y;
		device = d;
	};
	c.onBurn = [&](int x, int y) { burnX = x + 1000 * y; };
	c.onHurtThing = [&](int thing, int) { hurtThing = thing; };

	int px, py, pz;
	w.avatarPosition(px, py, pz);
	// a row of five open squares in the surface (no thing, a plain way for a missile)
	int bx = -1, by = -1;
	for (int dy = -40; dy <= 40 && bx < 0; ++dy)
		for (int dx = -40; dx <= 40 && bx < 0; ++dx) {
			int x0 = px + dx, y0 = py + dy;
			std::vector<std::pair<int, int>> path;
			bool ok = w.boltPath(0, x0, y0, x0 + 5, y0, 8, path);
			for (int i = 0; i <= 5 && ok; ++i) {
				std::vector<int> here;
				w.objectsAt(x0 + i, y0, 0, here);
				ok = here.empty() && !(w.flags.flags1[w.map.tile(0, x0 + i, y0) & 0x7FF] & 2);
			}
			if (ok && w.moveCost(x0, y0, 0) > 0) {
				bx = x0;
				by = y0;
			}
		}
	CHECK(bx >= 0);
	if (bx < 0)
		return 1;
	const int me = 1;
	w.moveObject(w.actorObject(me), bx, by, 0);
	// those of the party are out of the way
	for (int m = 1; m < w.actors.partySize; ++m)
		w.moveObject(w.actorObject(w.actors.partyRoster[m]), bx - 20, by - 20, 0);

	// out of range: bare hands reach one square
	int web = put(w, 0x11, 0, bx + 3, by, 0);
	CHECK(c.seAttack(me, 0x19A, bx + 3, by, true) == 2);
	// a web next to us is struck only with fire or a grenade; a lit torch sets it afire
	w.moveObject(web, bx + 1, by, 0);
	CHECK(c.seAttack(me, 0x19A, bx + 1, by, true) == 3);
	int torch = ready(w, me, 0xD1);
	burnX = -1;
	CHECK(c.seAttack(me, 0xD1, bx + 1, by, true) == 0);
	CHECK(burnX == bx + 1 + 1000 * by);
	w.deleteObject(torch);
	w.deleteObject(web);
	// a tree is struck only by the fire axe
	int tree = put(w, 0x107, 0, bx + 1, by, 0);
	CHECK(c.seAttack(me, 0x19A, bx + 1, by, true) == 3);
	int axe = ready(w, me, 0x76);
	hurtThing = -1;
	CHECK(c.seAttack(me, 0x76, bx + 1, by, true) == 0 && hurtThing == tree);
	w.deleteObject(axe);
	w.deleteObject(tree);

	// a bow wants arrows (0x2D): without them nothing is done; with them one is used up for a shot at an empty square
	int bow = ready(w, me, 0x1F);
	CHECK(c.seAttack(me, 0x1F, bx + 4, by, true) == 0);
	int arrows = w.addToInventory(me, 0x2D, 0, 3);
	CHECK(c.seAttack(me, 0x1F, bx + 4, by, true) == 0);
	CHECK(w.objects[arrows].quantity == 2);
	w.deleteObject(bow);
	w.deleteObject(arrows);

	// a spear shot (0x25) from the pack's last spear 0x1A lands on the square it was aimed at
	int thrower = ready(w, me, 0x25);
	int spear = w.addToInventory(me, 0x1A, 0, 1);
	CHECK(c.seAttack(me, 0x25, bx + 4, by, true) == 0);
	CHECK(count(w, 0x1A, bx + 4, by) == 1 && w.objects[spear].onMap());
	w.deleteObject(spear);
	w.deleteObject(thrower);

	// a spear 0x1A thrown from the hand lands where it was thrown and a spare from the pack is put in the hand
	int hand = ready(w, me, 0x1A);
	int spare = w.addToInventory(me, 0x1A, 0, 1);
	CHECK(c.seAttack(me, 0x1A, bx + 3, by, true) == 0);
	CHECK(w.objects[hand].onMap() && w.objects[hand].x == bx + 3);
	CHECK(w.objects[spare].deleted);
	int inHand = 0;
	{
		std::vector<int> inv;
		w.inventory(me, inv, false);
		for (int i : inv)
			inHand += !w.objects[i].deleted && w.objects[i].type == 0x1A && w.objects[i].location() == ObjRecord::READIED;
	}
	CHECK(inHand == 1);
	for (int i = 0; i < (int)w.objects.size(); ++i) // (tidy up)
		if (!w.objects[i].deleted && w.objects[i].type == 0x1A)
			w.deleteObject(i);

	// a grenade (0x89) thrown goes off where it lands; the next plain one is put in the hand; a device (0xF1) is a device
	int nade = ready(w, me, 0x89);
	int nade2 = w.addToInventory(me, 0x89, 0, 1);
	explodeX = -1;
	CHECK(c.seAttack(me, 0x89, bx + 4, by, true) == 0);
	CHECK(explodeX == bx + 4 && explodeY == by && !device && w.objects[nade].deleted && w.objects[nade2].deleted);
	{
		std::vector<int> inv;
		w.inventory(me, inv, false);
		int armed = 0;
		for (int i : inv)
			armed += !w.objects[i].deleted && w.objects[i].type == 0x89 && w.objects[i].location() == ObjRecord::READIED;
		CHECK(armed == 1);
		for (int i : inv)
			w.deleteObject(i);
	}
	int dev = ready(w, me, 0xF1);
	explodeX = -1;
	CHECK(c.seAttack(me, 0xF1, bx + 3, by, true) == 0);
	CHECK(explodeX == bx + 3 && device && w.objects[dev].deleted);

	// an arrow strikes a person: sooner or later the target is hurt (the hit test), the arrows are used up
	int foe = w.actors.partyRoster[1];
	w.actors.status[foe] = 0x20;
	w.moveObject(w.actorObject(foe), bx + 3, by, 0);
	w.actors.hp[foe] = 200;
	int bow2 = ready(w, me, 0x1F);
	int quiver = w.addToInventory(me, 0x2D, 0, 100);
	for (int i = 0; i < 60 && w.actors.hp[foe] == 200; ++i)
		c.seAttack(me, 0x1F, bx + 3, by, true, foe);
	CHECK(w.actors.hp[foe] < 200);
	CHECK(w.objects[quiver].quantity < 100);
	w.deleteObject(bow2);

	// a spear shot over water is lost where it comes down
	{
		int wx = -1, wy = -1;
		for (int dy = -60; dy <= 60 && wx < 0; ++dy)
			for (int dx = -60; dx <= 60 && wx < 0; ++dx) {
				int x0 = px + dx, y0 = py + dy;
				bool ok = true;
				for (int i = 0; i <= 4 && ok; ++i) {
					std::vector<int> here;
					w.objectsAt(x0 + i, y0, 0, here);
					ok = here.empty() && (w.flags.flags1[w.map.tile(0, x0 + i, y0) & 0x7FF] & 3) == 3;
				}
				if (ok) {
					wx = x0;
					wy = y0;
				}
			}
		if (wx >= 0) {
			w.moveObject(w.actorObject(me), wx, wy, 0);
			int thrower2 = ready(w, me, 0x25);
			int one = w.addToInventory(me, 0x1A, 0, 1);
			c.seAttack(me, 0x25, wx + 4, wy, true);
			CHECK(w.objects[one].deleted);
			w.deleteObject(thrower2);
		}
	}

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

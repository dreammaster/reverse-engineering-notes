// Passability: the port of GAME.EXE's sub_25D7F against the real map (skipped when the game data is absent).

#include "ultima6/movement.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::MovementClasses;
using ::Ultima6::Ultima6::MoveRules;
using ::Ultima6::Ultima6::Movement;
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
	// The class table: the listed types, in the original's order.
	CHECK(MovementClasses::classOf(0x16C) == 0 && MovementClasses::classOf(0x1AD) == 1);
	CHECK(MovementClasses::classOf(0x107) == 63 && MovementClasses::classOf(0x000) == -1);
	CHECK(MovementClasses::flags(-1) == 0x6000);
	CHECK(MovementClasses::flags(MovementClasses::classOf(0x171)) & MovementClasses::FLIES);   // bats fly
	CHECK(MovementClasses::flags(MovementClasses::classOf(0x15A)) & MovementClasses::SWIMS);   // sea serpents swim

	World world;
	if (!world.load("u6/game")) {
		std::printf("skip (no game data)\n");
		return failures ? 1 : 0;
	}
	Movement mv(world);
	MoveRules rules;
	int avatar = world.actorObject(1);
	CHECK(avatar >= 0);

	// Find a lone water tile and a lone grass tile on the surface with nothing standing on them.
	int wx = -1, wy = -1, gx = -1, gy = -1;
	std::vector<int> here;
	for (int y = 0; y < 1024 && (wx < 0 || gx < 0); ++y) {
		for (int x = 0; x < 1024; ++x) {
			int t = world.map.tile(0, x, y);
			int f = world.flags.flags1[t];
			world.objectsAt(x, y, 0, here);
			if (!here.empty())
				continue;
			if (wx < 0 && (f & 3) == 3)
				wx = x, wy = y;
			if (gx < 0 && t == 5) // grass
				gx = x, gy = y;
		}
	}
	CHECK(wx >= 0 && gx >= 0);

	// The avatar walks on grass but not on water.
	CHECK(mv.check(gx, gy, 0, avatar, rules).passable);
	CHECK(!mv.check(wx, wy, 0, avatar, rules).passable);

	// Other kinds of movers cross terrain the avatar cannot.
	World::Obj bat = world.objects[avatar];
	bat.type = 0x171;
	bat.actor = 200;
	bat.status = 0;
	world.objects.push_back(bat);
	int batIndex = (int)world.objects.size() - 1;
	CHECK(mv.check(wx, wy, 0, batIndex, rules).passable);

	World::Obj serpent = bat;
	serpent.type = 0x15A;
	world.objects.push_back(serpent);
	int serpentIndex = (int)world.objects.size() - 1;
	CHECK(mv.check(wx, wy, 0, serpentIndex, rules).passable);
	CHECK(!mv.check(gx, gy, 0, serpentIndex, rules).passable);

	// Lord British's throne room: the walls hold, and so does the king himself.
	int ax, ay, az;
	world.avatarPosition(ax, ay, az);
	int open = 0, shut = 0;
	for (int dy = -3; dy <= 3; ++dy)
		for (int dx = -3; dx <= 3; ++dx)
			if (dx || dy)
				(mv.check(ax + dx, ay + dy, az, avatar, rules).passable ? open : shut)++;
	CHECK(open > 0 && shut > 0);

	if (failures == 0)
		std::printf("ok (%d open, %d shut around the start)\n", open, shut);
	return failures ? 1 : 0;
}

#include "ultima6/equipment.h"

namespace Ultima6 {
namespace Ultima6 {

const int Equipment::SLOT_X[SLOTS] = {200, 176, 176, 176, 224, 224, 224, 200};
const int Equipment::SLOT_Y[SLOTS] = {16, 24, 40, 56, 24, 40, 56, 64};

// The thirty-three tiles of ds:07DD that go in a hand
static const int HAND_TILES[33] = {0x220, 0x221, 0x223, 0x224, 0x225, 0x226, 0x227, 0x22A, 0x22F, 0x230, 0x238, 0x254, 0x256, 0x255, 0x259, 0x262, 0x263,
								   0x264, 0x270, 0x271, 0x272, 0x273, 0x274, 0x275, 0x279, 0x27D, 0x27E, 0x27F, 0x280, 0x281, 0x2A2, 0x2A3, 0x2B9};

int Equipment::slotClass(int tile) {
	if (tile == 0x21A || tile == 0x21B)
		return 7;
	if (tile == 0x258 || (tile >= 0x37D && tile <= 0x37F))
		return 9;
	if (tile == 0x219 || (tile >= 0x250 && tile <= 0x252) || tile == 0x217 || tile == 0x101)
		return 1;
	if (tile >= 0x200 && tile <= 0x207)
		return 0;
	if ((tile >= 0x210 && tile <= 0x216) || tile == 0x218 || tile == 0x219 || tile == 0x28C || tile == 0x28E || tile == 0x29D || tile == 0x257)
		return 4;
	if (tile == 0x228 || tile == 0x229 || tile == 0x231 || tile == 0x235 || (tile >= 0x22B && tile <= 0x22E))
		return 8;
	if ((tile >= 0x208 && tile <= 0x20F) || tile == 0x222)
		return 5;
	for (int t : HAND_TILES)
		if (t == tile)
			return 2;
	return -1;
}

int Equipment::slotClassOf(const World &world, int index) {
	const World::Obj &o = world.objects[index];
	return slotClass(world.baseTile.firstTile[o.type] + o.frame);
}

// cmd_ready's choice of slot: a two-handed thing and a one-handed thing want the first hand, a shield the second; with a two-handed weapon in the first hand everything that goes in a
// hand looks there and finds it taken; an arm piece goes to the first arm, else the second.
// APPROX(72): the original keeps the state of the hands in word_4BF38, word_4BF3A and word_4BF3E, which a lit torch in the pack also sets (cmd_get, use_torch); here they are worked out
// from what is readied
int Equipment::target(const World &, const Slots &slots, int cls) {
	switch (cls) {
	case 8:
		return 2;
	case 2:
		if (slots.twoHanded)
			return 2;
		return slots.object[2] < 0 ? 2 : 5;
	case 5:
		return slots.twoHanded ? 2 : 5;
	case 9:
		return slots.object[3] < 0 ? 3 : 6;
	default:
		return cls;
	}
}

Equipment::Slots Equipment::of(const World &world, int actor) {
	Slots s;
	for (int &o : s.object)
		o = -1;
	s.twoHanded = false;
	std::vector<int> inv;
	world.inventory(actor, inv, false);
	for (int i : inv) {
		if (world.objects[i].location() != ObjRecord::READIED)
			continue;
		int cls = slotClassOf(world, i);
		if (cls < 0)
			continue;
		int slot = target(world, s, cls);
		s.object[slot] = i;
		if (cls == 8)
			s.twoHanded = true;
	}
	return s;
}

int Equipment::readiedWeight(const World &world, int actor) {
	std::vector<int> inv;
	world.inventory(actor, inv, false);
	int total = 0;
	for (int i : inv)
		if (world.objects[i].location() == ObjRecord::READIED)
			total += world.flags.block3[world.objects[i].type & (TileFlags::BLOCK3 - 1)];
	return total;
}

} // namespace Ultima6
} // namespace Ultima6

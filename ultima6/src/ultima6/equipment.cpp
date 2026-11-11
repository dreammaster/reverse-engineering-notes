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

// sub_20370: what Martian Dreams may ready is decided by the type of the thing (not by the tile): 1-5 head, 6-8 and 0xEA neck, 9-0xF feet, 0x11-0x17 and 0x5A body, 0x19-0x1E and 0x21-0x27 the
// arms (6 and 3), then three lists of types: ds:0E00 (guns, the cue stick, ..) class 8, ds:0E2C (tools and weapons, the tongs) class 2, ds:0E48 (lamps and torches) class 5
// APPROX(76): the original keeps the state of the hands in word_4E5B8 / word_4E5BE (sub_31E9C); here Equipment::target works it out from what is readied
static int mdSlotClass(int type) {
	static const uint16_t BIG[22] = {0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x34, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x191, 0xF0, 0xF1, 0x139, 0xA2, 0xA1, 0x28};
	static const uint16_t HAND[14] = {0x29, 0x2A, 0x31, 0x32, 0x33, 0x36, 0x37, 0x38, 0x10, 0xF2, 0x81, 0x105, 0x88, 0x66};
	static const uint16_t LIGHT[10] = {0x6D, 0x6F, 0x71, 0x73, 0x75, 0x6E, 0x70, 0x72, 0x74, 0x76};
	if (type >= 1 && type <= 5)
		return 0;
	if ((type >= 6 && type <= 8) || type == 0xEA)
		return 1;
	if (type >= 9 && type <= 0xF)
		return 7;
	if ((type >= 0x11 && type <= 0x17) || type == 0x5A)
		return 4;
	if (type >= 0x19 && type <= 0x1E)
		return 6;
	if (type >= 0x21 && type <= 0x27)
		return 3;
	for (uint16_t t : BIG)
		if (t == type)
			return 8;
	for (uint16_t t : HAND)
		if (t == type)
			return 2;
	for (uint16_t t : LIGHT)
		if (t == type)
			return 5;
	return -1;
}

int Equipment::slotClassOf(const World &world, int index) {
	const World::Obj &o = world.objects[index];
	if (world.game == GameId::MartianDreams)
		return mdSlotClass(o.type);
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
		if (world.objects[i].location() != ObjRecord::READIED || world.objects[i].type == 0x150 || world.objects[i].type == 0x151)
			continue; // (the charges of a staff are no equipment)
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

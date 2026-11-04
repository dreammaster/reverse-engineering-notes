#ifndef ULTIMA6_ULTIMA6_EQUIPMENT_H
#define ULTIMA6_ULTIMA6_EQUIPMENT_H

#include "ultima6/world.h"

namespace Ultima6 {
namespace Ultima6 {

/**
 * The places a character wears things (sub_1D766, sub_1DA30, cmd_ready): eight slots in the inventory view, 0 head, 1 neck, 2 and 5 the two hands (left, right), 3 and 6 the arms,
 * 4 the body, 7 the feet. What may go where is decided by the tile of the thing (tile numbers 0x200 and up are the wearable objects), see slotClass.
 */
struct Equipment {
	enum { SLOTS = 8 };
	// where each slot is drawn in the inventory view (x, y in the 320 x 200 screen), after sub_1DB44
	static const int SLOT_X[SLOTS];
	static const int SLOT_Y[SLOTS];

	/** sub_1D766: the kind of place a thing with this tile goes to: 0 head, 1 neck, 2 a one-handed thing, 4 body, 5 a shield, 7 feet, 8 a two-handed weapon, 9 an arm; -1 cannot be readied. */
	static int slotClass(int tile);
	static int slotClassOf(const World &world, int index);

	struct Slots {
		int object[SLOTS]; // index into World::objects, -1 for nothing
		bool twoHanded;    // a two-handed weapon holds both hands
	};
	/** What an actor has readied, by slot (sub_1DA30). */
	static Slots of(const World &world, int actor);
	/** The slot cmd_ready would put an object into (the slot may be taken). */
	static int target(const World &world, const Slots &slots, int cls);
	/** The weight of what is readied, by the bare weights of the types (word_4BF44 of sub_1DA30). */
	static int readiedWeight(const World &world, int actor);
};

} // namespace Ultima6
} // namespace Ultima6

#endif

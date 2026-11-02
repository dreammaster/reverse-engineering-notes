#ifndef ULTIMA6_ULTIMA6_STATS_VIEW_H
#define ULTIMA6_ULTIMA6_STATS_VIEW_H

#include "engine/framebuffer.h"
#include "ultima6/world.h"

#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/** The numbers GAME.EXE's panel shows for an actor (sub_2B4D1, sub_2B519, sub_2B549). */
struct ActorStats {
	static int level(const World &w, int actor) {
		return w.actors.level[actor];
	}
	/** 30 hit points a level, at least 1 and at most 255. */
	static int maxHealth(const World &w, int actor);
	/** The Avatar has twice his intelligence in magic points, the two kinds of mage (types 0x17A, 0x179 and 0x182) as much or half. */
	static int maxMagic(const World &w, int actor);
};

/**
 * The default panel of the play screen (sub_1D4DA): a party member's name over his or her portrait,
 * strength, dexterity, intelligence, magic, health, level and experience, and the four arrow
 * buttons below the portrait, at the 17 x 12 character window (176, 8) of the right-hand column.
 */
void drawStatsView(Framebuffer &screen, const World &world, int member);

/**
 * The inventory panel: the objects a party member carries as their 16 x 16 tiles in a grid of eight columns (readied ones first),
 * with the selected object's name and its weight under the grid and the member's carried weight against what he or she can carry.
 * `selected` indexes the list returned in `items` (indices into World::objects).
 */
/** The objects the inventory panel lists for a member: readied ones first. */
void inventoryItems(const World &world, int member, std::vector<int> &items);

void drawInventoryView(Framebuffer &screen, const World &world, int member, int selected, std::vector<int> &items);

/** The panel while talking: the speaker's portrait. */
void drawTalkView(Framebuffer &screen, const World &world, int actor);

} // namespace Ultima6
} // namespace Ultima6

#endif

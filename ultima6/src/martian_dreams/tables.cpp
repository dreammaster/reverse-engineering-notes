#include "ultima6/combat.h"
#include "ultima6/game_tables.h"

namespace Ultima6 {
namespace MartianDreams {

using ::Ultima6::Ultima6::CreatureStats;
using ::Ultima6::Ultima6::GameTables;
using ::Ultima6::Ultima6::TypeValue;

#include "martian_dreams/tables.inc"

const GameTables &tables() {
	static GameTables t;
	t.classTypes = MD_CLASS_TYPES;
	t.classFlags = MD_CLASS_FLAGS;
	t.classCount = sizeof(MD_CLASS_TYPES) / sizeof(MD_CLASS_TYPES[0]);
	t.creatures = MD_CREATURES;
	t.creatureCount = sizeof(MD_CREATURES) / sizeof(MD_CREATURES[0]);
	t.damage.assign(MD_DAMAGE, MD_DAMAGE + sizeof(MD_DAMAGE) / sizeof(MD_DAMAGE[0]));
	t.range.assign(MD_RANGE, MD_RANGE + sizeof(MD_RANGE) / sizeof(MD_RANGE[0]));
	t.armour.assign(MD_ARMOUR, MD_ARMOUR + sizeof(MD_ARMOUR) / sizeof(MD_ARMOUR[0]));
	t.eggType = 0x1D2;
	t.u6Creatures = false;
	return t;
}

} // namespace MartianDreams
} // namespace Ultima6

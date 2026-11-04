#include "ultima6/combat.h"
#include "ultima6/game_tables.h"

namespace Ultima6 {
namespace SavageEmpire {

using ::Ultima6::Ultima6::CreatureStats;
using ::Ultima6::Ultima6::GameTables;
using ::Ultima6::Ultima6::TypeValue;

#include "savage_empire/tables.inc"

const GameTables &tables() {
	static GameTables t;
	t.classTypes = SE_CLASS_TYPES;
	t.classFlags = SE_CLASS_FLAGS;
	t.classCount = sizeof(SE_CLASS_TYPES) / sizeof(SE_CLASS_TYPES[0]);
	t.creatures = SE_CREATURES;
	t.creatureCount = sizeof(SE_CREATURES) / sizeof(SE_CREATURES[0]);
	t.damage.assign(SE_DAMAGE, SE_DAMAGE + sizeof(SE_DAMAGE) / sizeof(SE_DAMAGE[0]));
	t.range.assign(SE_RANGE, SE_RANGE + sizeof(SE_RANGE) / sizeof(SE_RANGE[0]));
	t.armour.assign(SE_ARMOUR, SE_ARMOUR + sizeof(SE_ARMOUR) / sizeof(SE_ARMOUR[0]));
	t.eggType = 0xE6;
	t.u6Creatures = false;
	return t;
}

} // namespace SavageEmpire
} // namespace Ultima6

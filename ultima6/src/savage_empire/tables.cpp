#include "ultima6/combat.h"
#include "ultima6/game_tables.h"

namespace Ultima6 {
namespace SavageEmpire {

using ::Ultima6::Ultima6::CreatureStats;
using ::Ultima6::Ultima6::GameTables;

#include "savage_empire/tables.inc"

const GameTables &tables() {
	static GameTables t;
	t.classTypes = SE_CLASS_TYPES;
	t.classFlags = SE_CLASS_FLAGS;
	t.classCount = sizeof(SE_CLASS_TYPES) / sizeof(SE_CLASS_TYPES[0]);
	t.creatures = SE_CREATURES;
	t.creatureCount = sizeof(SE_CREATURES) / sizeof(SE_CREATURES[0]);
	return t;
}

} // namespace SavageEmpire
} // namespace Ultima6

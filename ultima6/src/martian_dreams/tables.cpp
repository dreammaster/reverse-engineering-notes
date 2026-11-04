#include "ultima6/combat.h"
#include "ultima6/game_tables.h"

namespace Ultima6 {
namespace MartianDreams {

using ::Ultima6::Ultima6::CreatureStats;
using ::Ultima6::Ultima6::GameTables;

#include "martian_dreams/tables.inc"

const GameTables &tables() {
	static GameTables t;
	t.classTypes = MD_CLASS_TYPES;
	t.classFlags = MD_CLASS_FLAGS;
	t.classCount = sizeof(MD_CLASS_TYPES) / sizeof(MD_CLASS_TYPES[0]);
	t.creatures = MD_CREATURES;
	t.creatureCount = sizeof(MD_CREATURES) / sizeof(MD_CREATURES[0]);
	return t;
}

} // namespace MartianDreams
} // namespace Ultima6

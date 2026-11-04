#include "ultima6/game_tables.h"

namespace Ultima6 {
namespace Ultima6 {

static GameTables g_active;
static bool g_chosen = false;

void selectTables(GameId id) {
	g_chosen = true;
	if (id == GameId::MartianDreams) {
		g_active = ::Ultima6::MartianDreams::tables();
	} else if (id == GameId::SavageEmpire) {
		g_active = ::Ultima6::SavageEmpire::tables();
	} else {
		g_active = GameTables();
		g_active.creatures = u6CreatureTable(g_active.creatureCount);
		u6ClassTable(g_active.classTypes, g_active.classFlags, g_active.classCount);
	}
}

const GameTables &activeTables() {
	if (!g_chosen)
		selectTables(GameId::Ultima6);
	return g_active;
}

} // namespace Ultima6
} // namespace Ultima6

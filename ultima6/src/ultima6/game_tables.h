#ifndef ULTIMA6_ULTIMA6_GAME_TABLES_H
#define ULTIMA6_ULTIMA6_GAME_TABLES_H

#include "engine/game_id.h"

#include <cstdint>

namespace Ultima6 {
namespace Ultima6 {

struct CreatureStats;

/**
 * The numbers each game keeps in its executable and the shared code asks for: the creature types with a movement class of their own and the class flags,
 * and the creature numbers (strength, dexterity, ...). Ultima VI's are in combat_tables.inc and movement_tables.inc; the spin-offs' were read from their
 * executables by tools/extract_creatures.py (src/martian_dreams/tables.inc, src/savage_empire/tables.inc).
 */
struct GameTables {
	const uint16_t *classTypes = nullptr;
	const uint16_t *classFlags = nullptr;
	int classCount = 0;
	const CreatureStats *creatures = nullptr;
	int creatureCount = 0;
};

/** Chooses the tables of a game (World::load does it); until then they are Ultima VI's. */
void selectTables(GameId id);
const GameTables &activeTables();

/** Ultima VI's own tables, which live where they are used. */
const CreatureStats *u6CreatureTable(int &count);
void u6ClassTable(const uint16_t *&types, const uint16_t *&flags, int &count);

} // namespace Ultima6
} // namespace Ultima6

namespace Ultima6 {
namespace MartianDreams {
const ::Ultima6::Ultima6::GameTables &tables();
}
namespace SavageEmpire {
const ::Ultima6::Ultima6::GameTables &tables();
}
} // namespace Ultima6

#endif

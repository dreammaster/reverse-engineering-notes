#include "savage_empire/game.h"

namespace Ultima6 {
namespace SavageEmpire {

const ::Ultima6::GameInfo &info() {
	static const ::Ultima6::GameInfo INFO = {
		::Ultima6::GameId::SavageEmpire, "Worlds of Ultima: The Savage Empire", "SAVAGE.EXE", "GAME.EXE", nullptr
	};
	return INFO;
}

} // namespace SavageEmpire
} // namespace Ultima6

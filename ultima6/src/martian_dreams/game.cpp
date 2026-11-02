#include "martian_dreams/game.h"

namespace Ultima6 {
namespace MartianDreams {

const ::Ultima6::GameInfo &info() {
	static const ::Ultima6::GameInfo INFO = {
		::Ultima6::GameId::MartianDreams, "Worlds of Ultima: Martian Dreams", "MARTIAN.EXE", "GAME.EXE", nullptr
	};
	return INFO;
}

} // namespace MartianDreams
} // namespace Ultima6

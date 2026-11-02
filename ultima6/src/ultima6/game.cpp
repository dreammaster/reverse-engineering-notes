#include "ultima6/game.h"

namespace Ultima6 {
namespace Ultima6 {

const ::Ultima6::GameInfo &info() {
	static const ::Ultima6::GameInfo INFO = {
		::Ultima6::GameId::Ultima6, "Ultima VI: The False Prophet", "U.EXE", "GAME.EXE", "END.EXE"
	};
	return INFO;
}

} // namespace Ultima6
} // namespace Ultima6

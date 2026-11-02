#ifndef ULTIMA6_ULTIMA6_GAME_H
#define ULTIMA6_ULTIMA6_GAME_H

#include "engine/game_id.h"

namespace Ultima6 {
namespace Ultima6 {

/**
 * Everything specific to Ultima VI: The False Prophet: main menu, intro, character creation,
 * ending sequence and the data that differs from the other two games.
 *
 * Note that this namespace has the same name as the engine namespace that
 * encloses it; inside it, refer to engine types as ::Ultima6::Name.
 */
const ::Ultima6::GameInfo &info();

} // namespace Ultima6
} // namespace Ultima6

#endif

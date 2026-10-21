#pragma once
// Character advancement (KEH checkLevelUps).
#include "fod/vm.h"

namespace fod {

// Promotes every member whose experience reached the next rank: hit-point gain and attribute points to spend.
void checkLevelUps(World &w, Host &h);

} // namespace fod

#pragma once
// Character advancement (KEH checkLevelUps).
#include "fod/vm.h"

namespace fod {

// Promotes every member whose experience reached the next rank: hit-point gain and attribute points to spend.
void checkLevelUps(World &w, Host &h);

// Shops and services (script ops 37 and 45).
void runShop(World &w, Host &h, int shopIndex);
void runService(World &w, Host &h, int serviceIndex);

} // namespace fod

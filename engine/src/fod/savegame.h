#pragma once
// Saving and loading (KEH writeSaveState / loadSaveState).
#include "fod/world.h"

namespace fod {

bool saveGame(World &w, const std::string &dir, int slot);
bool loadGame(const std::string &dir, int slot, Disk1 &disk, std::vector<std::pair<int, Bytes>> &maps);
int newestSave(const std::string &dir);   // slot (1..3) with the highest save counter, 0 if none

} // namespace fod

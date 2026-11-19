#pragma once
// The exploration main loop of WL.EXE (0x16B3C): the loop that runs while the party walks the map. It polls the keyboard (and the
// mouse through checkKeypress), dispatches the keys to the original handlers, advances the random encounter / idle timers and
// checks that somebody in the party is still able to act (otherwise the game over screen runs).
#include "wasteland1/machine.h"

namespace wl {

// 16B3C: runs until the game is over (returns after the game over routine, which restarts the program)
void explorationLoop(Machine &m);

} // namespace wl

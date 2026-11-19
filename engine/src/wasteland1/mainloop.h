#pragma once
// The exploration main loop of WL.EXE (0x16B3C): the loop that runs while the party walks the map. It polls the keyboard (and the
// mouse through checkKeypress), dispatches the keys to the original handlers, advances the random encounter / idle timers and
// checks that somebody in the party is still able to act (otherwise the game over screen runs).
#include "wasteland1/machine.h"

namespace wl {

// 16B3C: runs until the game is over (returns after the game over routine, which restarts the program)
void explorationLoop(Machine &m);

// 12760: a digit key shows the character screen of that party member
void characterKey(Machine &m, uint8_t key);
// 13AAF: asks what party member `member` should use and on whom (the USE command)
void useMember(Machine &m, uint8_t member);
// 13A80: the USE button: picks the member first
void useButton(Machine &m);

} // namespace wl

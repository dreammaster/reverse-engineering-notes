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

// 179B7: redraws the message window from the scroll-back buffer, the last line shown being `last`; 17A35 / 17A4D scroll it
void showMessageLines(Machine &m, uint8_t last);
void scrollMessagesUp(Machine &m);
void scrollMessagesDown(Machine &m);

// 163CA / 163C4 / 163A0: the redraw sequences after something changed on the map
void refreshMapAndStatus(Machine &m);
void refreshMessagesAndMap(Machine &m);
void refreshAfterMove(Machine &m);

// 1CB30: reloads the area when 4 minutes of game time have passed since entering it; 1656D: starts the idle event of the view
void checkEnteredTime(Machine &m);
void startIdleEvent(Machine &m);

// 161C0 (+ 1630C, 162CF, 162E1): the title screen with its credit pages; never returns (START runs the game)
void titleView(Machine &m);
void titleButtons(Machine &m);
void titleWait(Machine &m, uint8_t ticks);
void titleDelay(Machine &m, uint8_t seconds);

// 1C570: the game over screen (the program restarts after a key)
void gameOver(Machine &m);
// 16B31: enters the exploration loop; `sp` is remembered for the routines that restart it
void beginExploration(Machine &m, uint16_t sp);
// 16330: START on the title screen: loads the game and begins the exploration
void loadGameData(Machine &m, uint16_t sp);

} // namespace wl

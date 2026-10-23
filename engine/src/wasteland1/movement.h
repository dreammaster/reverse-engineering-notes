#pragma once
// Moving the party over the map (WL.EXE 0x1649E..0x1676A and 0x13E9B..0x13EC0).
#include "wasteland1/machine.h"

namespace wl {

// 1676A: one step of game time: the clock advances by the map's rates, the step counter ticks, health changes
void advanceTime(Machine &m);
// 13E9B: the cell (x, y) holds an interactive object (class 2); runs its action. True (CF) when it blocks the way
bool cellBlockedByObject(Machine &m, uint8_t x, uint8_t y);
// 16AD5: stepping onto a class 0Ah cell with the confirmation flag asks "really?"; true (CF) when the player says no
bool confirmGate(Machine &m, uint8_t x, uint8_t y);
// 15CE0: cell classes 0Bh and 4 may replace their own cell by the record's change; always true (CF)
bool applyStepOnChange(Machine &m, uint8_t x, uint8_t y);
// 1649E: can the party step in direction `dir` (0 up, 1 down, 2 left, 3 right, 4 stay, 5..8 diagonals)? True (CF) when not
bool stepBlocked(Machine &m, uint8_t dir);
// 164ED: the cell reached from (x, y) in direction `dir`
void stepTarget(uint8_t dir, uint8_t &x, uint8_t &y);
// 169BC: redraws the text window layout when the view is shown and the window state needs it
void refreshViewLayout(Machine &m);
// 1652D: moves the party one cell in direction `dir` (scrolls the view, advances time); false (CF) when nobody can move
bool moveParty(Machine &m, uint8_t dir);
// 1651A: tries to step in direction `dir` (checks the way first); true (CF) when blocked
bool tryStep(Machine &m, uint8_t dir);

} // namespace wl

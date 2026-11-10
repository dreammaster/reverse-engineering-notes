#pragma once
// The mouse of WL.EXE (0x18860 / 0x18928): the position and buttons come from the Input event state (input.h) instead of the
// DOS mouse driver; the hot spot table (ds:CAEB, 22 entries of left, top, right, bottom, handler) maps clicks to key codes by
// running the original handlers (cursor shape, buttons, roster rows) and the original cursor drawing.
#include "wasteland1/machine.h"

namespace wl {

// 18928: reads the mouse state; true (CF) when something changed since the last call (ds:7DF3..7DFB is updated)
bool mouseChanged(Machine &m);
// 18860 without its leading pollSpeedKeys: true (CF) with `al` set when a click on a hot spot produced a key code
bool pollMouse(Machine &m, uint8_t &al);

} // namespace wl

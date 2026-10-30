#pragma once
// The roster window at the bottom of the map screen: one line per member of the active group.
#include "wasteland1/machine.h"

namespace wl {

// 1708B: draws the line of the current character (position ds:B242): number, name, level, ammunition, hit points, condition and
// weapon
void drawRosterRow(Machine &m);
// 17029 / 1702E / 17033: redraws the line of party member `position`; `inverse` is the text mode of the line (FFh inverted)
void redrawRosterRow(Machine &m, uint8_t position, uint8_t inverse);
// 16F70: draws the frame of the roster window and then every line
void drawRosterFrame(Machine &m);

// 1728C: shows the roster window (frame and all lines)
void drawRosterHeadersAndMembers(Machine &m);
// 17923: shows the title page text `page` in the picture window
void showTitleText(Machine &m, uint8_t page);

} // namespace wl

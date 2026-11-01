#pragma once
// The hot key button row at the bottom of the screen ("USE ENC ORDER ..."): a text of space separated words, the selected button,
// a table of handlers per button (WL.EXE 0x17451..0x17680).
#include "wasteland1/machine.h"

namespace wl {

// 17564: textString = the button text of the current row (ds:468A selects the row)
void getButtonsText(Machine &m);
// 17533: finds the position of button ds:4688 in the text (stored in ds:468B); false (CF) when there is no such button
bool findButton(Machine &m);
// 173D2: draws the whole row and highlights the selected button
void drawButtonsRow(Machine &m);
// 17451 / 1744C: draws the selected button plain / highlighted
void drawButton(Machine &m, bool highlight);
// 17574: handles the key in ds:4667: a letter, the cursor keys (F3h / F4h) or ENTER selects and runs a button
void buttonKey(Machine &m);

} // namespace wl

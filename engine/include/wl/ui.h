#pragma once
// Ports of the low-level text and glyph routines of WL.EXE (operating on a wl::Machine image).
#include "wl/machine.h"

namespace wl {

// 8x8 monochrome glyph at (rectTextX, rectTextY); white on black, inverted when inverseText is set. (WLA 0x1060C)
void writeCharacterAt(Machine &m, uint8_t ch);

// 8x8 colour glyph (COLORF.FNT) at (stringXPos, stringYPos); advances stringXPos. (WLA 0x10CB6)
void writeColorCharacter(Machine &m, uint8_t ch);

// Clears the rectangle [rectLeftCol..rectRightCol] x [rectTopLine..rectBottomLine) and homes the text cursor. (WLA 0x10762)
void clearRect(Machine &m);

} // namespace wl

#pragma once
// Ports of the low-level text and glyph routines of WL.EXE.
//
// Part 1 works on a wl::Machine image (the byte-exact test scaffolding used by wltest); part 2 (class Ui) is the
// clean implementation used by the engine, drawing into a wl::Video.
#include "wl/assets.h"
#include "wl/machine.h"
#include "wl/video.h"
#include <string>

namespace wl {

// ---- machine image ports (verified against the original with wltest) --------------------------------------------

// 8x8 monochrome glyph at (rectTextX, rectTextY); white on black, inverted when inverseText is set. (WLA 0x1060C)
void writeCharacterAt(Machine &m, uint8_t ch);

// 8x8 colour glyph (COLORF.FNT) at (stringXPos, stringYPos); advances stringXPos. (WLA 0x10CB6)
void writeColorCharacter(Machine &m, uint8_t ch);

// Clears the rectangle [rectLeftCol..rectRightCol] x [rectTopLine..rectBottomLine) and homes the text cursor. (WLA 0x10762)
void clearRect(Machine &m);

// ---- engine ------------------------------------------------------------------------------------------------------

// Character-cell screen painter: the window frame, button row and gauges the original builds out of COLORF.FNT glyphs.
class Ui {
public:
	Ui(const Assets &a, Video &v) : _a(a), _v(v) {}

	// cursor of the colour glyph writer (IDB _stringXPos/_stringYPos)
	int cx = 0, cy = 0;
	bool buttonLettersAlt = true;      // _saved+0FEh == 0: button letters use the second colour set (+28)

	void colorChar(uint8_t glyph);                         // writeColorCharacter
	void monoChar(int x, int y, uint8_t ascii, bool inverse = false);   // writeCharacterAt with code ascii-0x20

	void clearScreen() { _v.clear(); }
	void clearRectCells(int leftCol, int rightCol, int topLine, int bottomLine);

	// frame pieces
	void box(int x0, int y0, int x1, int y1);              // sub_19812: single-line window frame
	void mainFrame();                                      // sub_197BB: the map window / message window frame
	void titleLetters();                                   // sub_166D3: vertical WASTELAND on the right edge
	void clockAndGauge(int hours, int minutes, int radLevel, bool hasDetector, bool drawClock);   // sub_17DF1
	void buttonsRow(const std::string &text, int selected = -1);   // drawButtonsRow / sub_17451 highlight

private:
	const Assets &_a;
	Video &_v;
};

} // namespace wl

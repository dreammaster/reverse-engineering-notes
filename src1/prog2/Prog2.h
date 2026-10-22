/* Yendor1::Prog2 -- PROG2.EXE, the prologue: five book pages of story text, each with a small animated picture. The first two
 * pages leave after a timeout (20 s and 5 s) with a page-curl and a pixel dissolve; the last three animate until the "More" icon
 * is clicked. SW.EXE starts it with the intro-mode flag in parm_03 (1 = the Esc key is allowed to skip the whole prologue).
 * Function names map to the disassembly (prog2.idb): see the comments on each routine. The page texts and the animation tables
 * are read from PROG2.EXE at run time. */

#ifndef YENDOR1_PROG2_PROG2_H
#define YENDOR1_PROG2_PROG2_H

#include "data/ExeImage.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Ui.h"

#include <vector>

namespace Yendor1 {
namespace Prog2 {

using Shared::byte;
using Shared::uint16;
using Shared::uint32;

class Prog2 {
public:
	Prog2(Shared::Runtime &rt, const Shared::Params &params);

	/** Run the prologue; returns the DOS exit code (0 normal, 1 skipped with Esc, FFh when the parameter block is invalid). */
	int run();

	/** The generator the animations and the dissolve draw from (tests give it the original run's seed). */
	Shared::Rng &rng() { return _ui.rng; }

private:
	// sub_102FC .. sub_11392, in address order
	uint16 stepAnimation(uint16 entry);       // sub_102FC: one chain entry; returns the next entry
	void pageCurl();                          // sub_103AF (the part after the timeout)
	void firstPageWait();                     // sub_103AF
	void secondPage();                        // sub_1045A
	void thirdPageLoop();                     // sub_10548
	void fourthPageLoop();                    // sub_1059F
	void fifthPageLoop();                     // sub_105BC
	void hideCursorIfOver();                  // sub_105E4
	void clearTextArea();                     // sub_10625
	void drawPage1();                         // sub_10650
	void drawPage2();                         // sub_10737
	void drawPage3();                         // sub_10809
	void drawPage4();                         // sub_108CA
	void drawPage5();                         // sub_1098B
	void scaleDelays();                       // sub_10A7B
	void curlFrame();                         // sub_10AD5
	void kettleAnimation();                   // sub_10BD2
	void pixelDissolve();                     // sub_11392 (with shared_27c37f, shared_1f8a83, shared_33aaab)
	byte waitWithTimeout(int seconds);        // sub_11750
	byte blockingWait();                      // shared_21818f with BL=0
	bool skipped() const;                     // the "parm_03 == 1 and Esc" test between pages

	// helpers
	void tile(uint16 x, uint16 y, uint16 index);
	void drawText(uint16 textOffset, int lines, byte row);
	uint16 word(uint16 dsOffset) const;                        // a word of the animation table copied from the exe (DS:1641h..)
	void setWord(uint16 dsOffset, uint16 value);
	void draw();                                               // tileOp 1: tile _tileIndex at (x1, y1)
	void drawRun(int count);                                   // drawTileRun

	Shared::Runtime &_rt;
	Vid::Context &_c;
	Shared::BookUi _ui;
	Shared::Params _params;
	Data::ExeImage _exe;
	bool _haveExe;
	std::vector<byte> _table;        // DS:1641h.. : the chain pointers followed by the animation entries (18 bytes each)
	uint16 _tileIndex;               // word_11E85: the tile the next draw uses
	uint16 _inputKeys;               // word_11E7F: AH = key mask, AL = icon mask for the wait loops
	byte _state;                     // byte_11E90: the last input result, finally the exit code
	byte _cursorHidden;              // byte_11E8B
	uint16 _curlOffset, _curlLeft, _curlRight, _curlFrame;   // word_11E81, word_11E89, word_11E8E, word_11E8C
	bool _quit;
};

} // namespace Prog2
} // namespace Yendor1

#endif

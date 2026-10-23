/* Yendor1::Prog3 -- PROG3.EXE, the book menu (the "Table of Contents"): Create / View / Remove a character, Assemble the party,
 * Open the book (play), the Bookmarker (save/restore), About the Authors, Introduction and Close the book. SW.EXE starts it and
 * acts on its exit code (the number of the entry, 7 About, 8 Introduction, 9 Close; 5 starts the game). Entries 1-4 and 6 start PROG5
 * and PROG6 as children.
 * Function names map to the disassembly (prog3.idb): see the comments on each routine. Texts and the hit boxes are read from
 * PROG3.EXE at run time. */

#ifndef YENDOR1_PROG3_PROG3_H
#define YENDOR1_PROG3_PROG3_H

#include "data/ExeImage.h"
#include "shared/Launcher.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Ui.h"

namespace Yendor1 {
namespace Prog3 {

using Shared::byte;
using Shared::uint16;

class Prog3 {
public:
	Prog3(Shared::Runtime &rt, const Shared::Params &params, Shared::Launcher &launcher);

	/** Run the menu; returns the DOS exit code (the selected entry). */
	int run();

private:
	void drawMenuPage();                       // sub_1053F
	void highlight(byte entry, byte colour);   // shared_0bb58f: draw (or erase) the frame round entry 1..9
	void chooseEntry();                        // sub_105BB: arrow keys / mouse until Enter or a click
	void message(uint16 boxOffset, uint16 textOffset, byte row);   // loc_10441: a one-line box that any key closes
	void confirmClose();                       // the "Are you certain that you want to EXIT?" panel
	int runChild(int program);
	bool readPartyHeader();                    // fileReadRecordTo(PARTY.DAT record 0)
	uint16 headerWord(int offset) const;
	const byte *data(uint16 dsOffset, uint32_t length) const;
	const char *text(uint16 dsOffset) const;

	Shared::Runtime &_rt;
	Vid::Context &_c;
	Shared::BookUi _ui;
	Shared::Params _params;
	Shared::Launcher &_launcher;
	Data::ExeImage _exe;
	bool _haveExe;
	byte _party[100];       // record 0 of CURRGAME\PARTY.DAT (the original reads it to DS:000Ch)
	byte _entry;            // byte_126D6: the highlighted entry, finally the exit code
	bool _quit;
};

} // namespace Prog3
} // namespace Yendor1

#endif

/* Yendor1::Prog6 -- PROG6.EXE, the Bookmarker: a small popup over the book with "Save and continue to play", "Save and return to DOS",
 * "Restore from last save" and, in a running game, "Return to DOS without saving" and "Make camp or rest at an inn". It asks for a
 * confirmation, starts PROG0 (the save slot program) to do the saving or restoring, and exits with the number of the choice (0 = cancelled).
 * parm_02: 0 = called from the book menu (3 entries), 1 = called from the game (4 or 5 entries).
 * Function names map to the disassembly (prog6.idb). Texts, boxes and hit boxes are read from PROG6.EXE at run time. */

#ifndef YENDOR1_PROG6_PROG6_H
#define YENDOR1_PROG6_PROG6_H

#include "data/ExeImage.h"
#include "shared/Launcher.h"
#include "shared/Params.h"
#include "shared/Runtime.h"
#include "shared/Ui.h"

#include <vector>

namespace Yendor1 {
namespace Prog6 {

using Shared::byte;
using Shared::uint16;

class Prog6 {
public:
	Prog6(Shared::Runtime &rt, const Shared::Params &params, Shared::Launcher &launcher);

	/** Returns the DOS exit code: the chosen entry, 0 when cancelled, FDh for a bad mode in parm_02. */
	int run();

private:
	void saveBackground();                     // sub_10550: keep the page area under the popup
	void restoreBackground();                  // sub_10513
	void chooseEntry();                        // sub_1058D
	bool confirm();                            // sub_104BF: true when Enter was pressed
	int saveOrRestore(int mode, const char *message);   // loc_10322 / loc_103C6: PROG0 does the work
	void leave(bool restoreArea);              // loc_102C7 / loc_102DE
	void drawPartyStrip(const byte *record);   // not ported yet
	bool readPartyRecord(uint16 record, byte *buf);
	const byte *data(uint16 dsOffset, uint32_t length) const;
	const char *text(uint16 dsOffset) const;

	Shared::Runtime &_rt;
	Vid::Context &_c;
	Shared::BookUi _ui;
	Shared::Params _params;
	Shared::Launcher &_launcher;
	Data::ExeImage _exe;
	bool _haveExe;
	std::vector<byte> _saved;       // the area under the popup (word_127EC: a DOS block in the original)
	byte _party[100];               // record 0 of PARTY.DAT
	byte _entry;                    // byte_128AF
	byte _count;                    // byte_128B0
	byte _state;                    // byte_127EB: the last input result
	bool _quit;
};

} // namespace Prog6
} // namespace Yendor1

#endif

/* Yendor1::Prog0 -- PROG0.EXE, the saved-game copier. It never draws with the video driver; it works on the DOS text screen and on
 * files: the current game lives in CURRGAME\ (PARTY.DAT, NPC.DAT, AREA.DAT) and a saved game in SAVEGAME.<x>\ with the same three files.
 *   parm_0E = 1: SW.EXE at the start of a game: list the saved parties, let the player pick one (nothing to ask when there is
 *                one), and copy it into CURRGAME.
 *   parm_0E = 2: PROG6 saving: CURRGAME -> SAVEGAME.<parm_10>.     parm_0E = 3: restoring: SAVEGAME.<parm_10> -> CURRGAME.
 * Exit code 0 = done, 21h = no saved game found, 20h = no (valid) party chosen, 1/3 = a CURRGAME file cannot be opened, 9/0Ah/0Bh = the
 * saved game's PARTY/NPC/AREA file cannot be opened, FEh = bad mode, FFh = bad parameter block.
 * Function names map to the disassembly (prog0.idb). */

#ifndef YENDOR1_PROG0_PROG0_H
#define YENDOR1_PROG0_PROG0_H

#include "shared/Params.h"
#include "shared/Runtime.h"

#include <string>
#include <vector>

namespace Yendor1 {
namespace Prog0 {

using Shared::byte;
using Shared::uint16;

class Prog0 {
public:
	Prog0(Shared::Runtime &rt, const Shared::Params &params);

	int run();

private:
	std::vector<char> findSlots();                 // FINDFIRST/FINDNEXT "SAVEGAME.*": the extension letters
	int chooseSlot(const std::vector<char> &slots, char &chosen);   // mode 1: the list and the prompt
	int openAll(char slot);                        // sub_103C0 / sub_103AC: every file must open
	int copyGame(bool restore, char slot);         // loc_101D9 (SAVEGAME -> CURRGAME) and loc_10286 (CURRGAME -> SAVEGAME)
	std::string path(const char *dir, const char *file) const;
	void text(int col, int row, const char *s);

	Shared::Runtime &_rt;
	Shared::Params _params;
};

} // namespace Prog0
} // namespace Yendor1

#endif

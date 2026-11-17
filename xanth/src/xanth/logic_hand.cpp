// Functions of the original that the decompiler turns into wrong control flow; written by hand from the original's code.
#include "xanth/logic/logic.h"

namespace Xanth {

// cmdWait: waits until the next scheduled event of a list of "interesting" events if one is pending in this room, else one turn. The list the original
// walks (seg 0x32DF, 15 entries of 4 bytes {event id, room}) does not hold event ids in the CD version's data - it overlaps a table of far pointers to text -
// so in practice no entry matches and every WAIT is one turn (as the original behaves).
int cmdWait(Game &g) {
	Runtime &rt = g.rt;
	int turns = 0;
	rt.word_85222 = 0;
	for (int i = 0; i < 15; i++) {
		int id = g.exe16(0x32DF, i * 4);
		int room = g.exe16(0x32DF, i * 4 + 2);
		if (g.event_pending(id) && (room == g.state.v.current_room || room == 0)) {
			turns = g.sub_1AC45(g.exe16(0x32DF, i * 4)) + 1;
			break;
		}
	}
	if (turns == 0)
		turns = 1;
	if (!g.event_pending(5))
		g.Font_WriteMessage("Time passes.");
	g.advance_time(1, turns);
	return 1;
}

} // namespace Xanth

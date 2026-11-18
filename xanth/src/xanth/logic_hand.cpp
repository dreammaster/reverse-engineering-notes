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

// sub_44130: removes a verb from the verb list of a thing (the entries after it move up).
int sub_44130(Game &g, Word obj, Word verb) {
	int p = g.verbListOffset(obj);
	if (!p)
		return 0;
	int found = 0;
	while (g.exe16(0x3000, p) != 0) {
		if (found) {
			g.exe16(0x3000, p) = g.exe16(0x3000, p + 2);
		} else if (g.exe16(0x3000, p) == verb) {
			found = 1;
			if (g.exe16(0x3000, p + 2) == 0) {
				g.exe16(0x3000, p) = 0;
				return verb;
			}
		}
		p += 2;
	}
	return found ? 0 : 0;
}

// objh_390 (Stanley's cellar door / the sliding blocks): the decompiler cannot follow the pointer into the verb list table.
int objh_390(Game &g, Word phase) {
	if ((uint16)(phase - 8) > 0x10)
		return 0;
	switch (phase) {
	case 8:
		g.call_object_handler(0x1AE, 0xA);
		g.Font_WriteMessage(MSG(72, 1));
		break;
	case 9:
	case 10:
	case 11:
		g.Font_WriteMessage(MSG(72, 0));
		break;
	case 13:
		if (int slot = g.verbListOffset(0x1AF))
			g.exe16(0x3000, slot) = 0x1A;
		break;
	case 14:
		if (!g.object_has_attr(0x1AE, 0xA)) {
			g.object_set_attr(0x1AE, 0xA);
			g.wputchar(0xB);
			g.wait_ticks_or_click(0x1388);
			g.showCutscene(4);
		}
		break;
	case 15:
	case 24:
		g.object_list_append(0x1AF);
		break;
	default:
		break;
	}
	return 0;
}

} // namespace Xanth

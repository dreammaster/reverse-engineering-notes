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
		g.Font_WriteMessage("Time passes. ");
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

// Visibility callbacks of overlay records (the decompiler cannot tell how many arguments these small thunks take).
int loc_4E632(Game &g) {
	return g.object_has_attr(0x34, 0xE) == 0;
}

int sub_312E3(Game &g) {
	return g.object_has_attr(0xCA, 0xE);
}

int sub_312ED(Game &g) {
	return g.object_has_attr(0xCA, 0xE);
}

// number_to_words: "three hundred and forty two". The words are those of the table the original keeps next to its direction names (1-19, then the tens); " thousand",
// " hundred" and " and " join them, tens and units are separated by a space.
static const char *numberWord(int v) {
	static const char *const small[20] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
	                                      "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
	static const char *const tens[10] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
	if (v >= 1 && v <= 19)
		return small[v];
	if (v >= 20 && v <= 90 && v % 10 == 0)
		return tens[v / 10];
	return nullptr;
}

TextRef number_to_words(Game &g, Word arg0) {
	int n = (int16)arg0;
	std::string out;
	if (n == 0) {
		out = "zero";
	} else if (numberWord(n) && n <= 255) {
		out = numberWord(n);
	} else {
		int th = n / 1000, h = (n - 1000 * th) / 100, rem = n - 100 * (h + 10 * th);
		if (th != 0 && numberWord(th))
			out += std::string(numberWord(th)) + " thousand";
		if (h != 0 && numberWord(h))
			out += std::string(th ? " " : "") + numberWord(h) + " hundred";
		if (rem != 0) {
			if (h != 0 || th != 0)
				out += " and ";
			if (numberWord(rem)) {
				out += numberWord(rem);
			} else {
				int ten = (rem / 10) * 10, one = rem % 10;
				if (ten != 0 && numberWord(ten))
					out += numberWord(ten);
				if (one != 0 && numberWord(one))
					out += std::string(ten ? " " : "") + numberWord(one);
			}
		}
	}
	return TextRef(g.keepText(out));
}

// The visibility callbacks of the maze pictures (overlay records): whether the cell the player is in (flag_852AA / flag_852AC) has a wall bit set in the table at DS:2C91.
static int mazeBit(Game &g, int bit) {
	int cell = ((g.state.v.flag_852AA_exit_fn_12 * 5 * 2) + g.state.v.flag_852AC_exit_fn_12) * 2;
	return (g.exe8(0x84FC, 0x2C91 + cell) & (1 << bit)) ? (0x100 << bit) : 0;
}

int loc_5F405(Game &g) { return mazeBit(g, 0); }
int loc_5F421(Game &g) { return mazeBit(g, 1); }
int loc_5F4AD(Game &g) { return mazeBit(g, 2); }
int loc_5F4C9(Game &g) { return mazeBit(g, 3); }
int loc_5F4E5(Game &g) { return mazeBit(g, 4); }
int loc_5F501(Game &g) { return mazeBit(g, 5); }

} // namespace Xanth

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

// sub_1C4ED(root, callback, visibleOnly): walks a thing and everything inside it, calling callback(object, depth, kind) until it returns non-zero (the program's own container walk;
// the decompiler cannot follow its local stack frame).
int sub_1C4ED(Game &g, Word root, Word callback, Word callbackSegment, Word visibleOnly) {
	typedef int (*WalkFn)(Game &, Word, Word, Word);
	(void)callbackSegment;
	WalkFn fn = (WalkFn)callback.v;
	if (!fn || !(int)root)
		return 0;
	g.walkContentsEx(root, (int)visibleOnly != 0, [&](int obj, int depth, int kind) { return fn(g, obj, depth, kind) != 0; });
	return 0;
}

// event_fn_25 (event 26): the countdown of the cellar scene - a message each step (when the player is in room 0xFB), the next step of the event scheduled, and the two properties of object
// 0xD9 / 0xFE set from a table. (The decompiler cannot follow the local tables it indexes with the step.)
int event_fn_25(Game &g) {
	Vars &v = g.state.v;
	static const uint16 message[5] = {0, 0x4A, 0x4B, 0x4C, 0x4D};
	static const uint16 propA[5] = {0, 0, 0x155, 0x156, 0};
	static const uint16 propB[5] = {0, 0, 0x160, 0x161, 0};
	v.word_85280++;
	unsigned n = v.word_85280 < 5 ? v.word_85280 : 0;
	if (v.current_room == 0xFB)
		g.Font_WriteMessage(" %s.", TextRef::fromWords(message[n], 0xF021));
	if (v.word_85280 < 3)
		g.schedule_event(0x1A, 1);
	else
		g.schedule_event(0x1D, 2);
	if (propA[n]) {
		g.object_set_prop_a(0xD9, propA[n]);
		g.object_set_prop_a(0xFE, propB[n]);
	}
	return 1;
}

// sub_1AAFB: empties the event queue.
int sub_1AAFB(Game &g) {
	g.state.events.clear();
	return 0;
}

// sub_5F96C (the random message of the end is in the data segment 0x5FF4 of its overlay): the description of what is around in the barrow maze (printed after the room's own text): a sentence chosen by where the player is, built in a text buffer and
// shown at the end. (The decompiler loses the segments of the text pointers where the paths join.)
int sub_5F96C(Game &g) {
	Vars &v = g.state.v;
	Runtime &rt = g.rt;
	char *msg = rt.word_81A66;
	memcpy(msg, FAR(DS, 0x2DB2), 3);
	auto add = [&](TextRef id) { strcat(msg, g.ResourceText_GetStr(id)); };
	if (sub_5F1A7(g, 0)) {
		if (sub_5F212(g, 0)) {
			add(MSG(29, 12));
		} else if (v.flag_852D1_agony_moss != 0) {
			add(MSG(29, 15));
		} else {
			add(MSG(29, 13));
			if (g.object_parent_is(0xCE, 0xCD))
				add(MSG(29, 14));
			strcat(msg, FAR(DS, 0x2DB5));
		}
	} else if (sub_5F275(g, 0) && g.object_parent_is(0xD9, 0xCD)) {
		add(MSG(29, 16));
	} else if (sub_5F28C(g, 0) && g.object_parent_is(0xDA, 0xCD)) {
		add(MSG(29, 17));
	} else if (sub_5F2A3(g, 0)) {
		add(MSG(29, 18));
	} else if (sub_5F2C8(g, 0)) {
		add(MSG(29, 19));
		if (g.object_parent_is(0x58, 0xD4))
			add(MSG(29, 20));
	} else if (sub_5F2FE(g, 0)) {
		add(MSG(29, 21));
	} else if (!sub_5F315(g, 0)) {
		strcat(msg, g.ResourceText_GetStr(g.exeText(0x5FF4, (((uint16)(g._rand()) % (uint16)5) * 4) + 0x504)));
	} else if (sub_5F32C(g, 0)) {
		int n = 0;
		for (int off = 0x5E4; off < 0x604; off += 2)
			if (g.object_has_attr(g.exe16(0x3263, off), 0xA))
				n++;
		add(n == 0 ? MSG(29, 23) : MSG(29, 22));
		add(MSG(29, 24));
	} else if (sub_5F34B(g, 0)) {
		add(MSG(29, 25));
		if (g.object_has_attr(0xD7, 0xE))
			add(MSG(29, 26));
	}
	g.Font_WriteMessage(msg);
	return 0;
}

// conv_text_77: the hint for the recipe (the lock's seven digits are the ingredients): what the player has put in (the digits that are not 0), then what is wrong (the digits that
// differ from the recipe at DS:1B8A). The decompiler nests the loops of this function; they follow each other. The words are those of the data segment.
TextRef conv_text_77(Game &g) {
	Vars &v = g.state.v;
	Runtime &rt = g.rt;
	const int kDs = 0x84FC;
	uint16 guess[7], recipe[7];
	for (int i = 0; i < 7; i++) {
		guess[i] = v.byte_8B9D2[i];
		g.exe16(kDs, 0x5DEC + 2 * i) = guess[i];
		recipe[i] = g.exe16(kDs, 0x1B8A + 2 * i);
	}
	int firstWrong = -1, lastWrong = -1, firstSet = -1, lastSet = -1, right = 0, wrong = 0;
	for (int i = 0; i < 7; i++) {
		if (guess[i] != recipe[i]) {
			if (firstWrong < 0)
				firstWrong = i;
			lastWrong = i;
			wrong++;
		} else {
			right++;
		}
		if (guess[i] != 0) {
			if (firstSet < 0)
				firstSet = i;
			lastSet = i;
		}
	}
	char *msg = rt.word_81A66;
	msg[0] = 0;
	auto add = [&](const char *t) { strcat(msg, t); };
	auto msgText = [&](int index) { return g.ResourceText_GetStr(TextRef::fromWords(index, 0xF037)); };
	auto ds = [&](int off) { return (const char *)FAR(kDs, off); };
	if (lock_combination_correct(g)) {
		rt.voice_track = 0x529;
		strcpy(msg, msgText(0));
		return msg;
	}
	if (!g.object_parent_is(0x9B, 1)) {
		rt.voice_track = 0x52A;
		strcpy(msg, msgText(1));
		return msg;
	}
	rt.voice_track = 0xFFFF;
	strcpy(msg, msgText(2));
	if (firstSet < 0) {
		add(msgText(3));
	} else {
		for (int i = firstSet; i < 7; i++) {
			if (guess[i] == 0)
				continue;
			if (i != firstSet)
				add(ds(0x1B98));
			if (i == lastSet && right > 1)
				add(ds(0x1B9B));
			switch (i) {
			case 0:
				add(ds(v.byte_8B9D2[0] == 1 ? 0x1BA0 : 0x1BA9));
				add(ds(0x1BB4));
				break;
			case 1:
				add(msgText(4));
				break;
			case 2:
				add(v.byte_8B9D2[2] == 1 ? ds(0x1BC3) : v.byte_8B9D2[2] == 2 ? ds(0x1BCA) : v.byte_8B9D2[2] == 3 ? ds(0x1BD4) : msgText(5));
				add(ds(0x1BE0));
				break;
			case 3:
				add(ds(0x1BEA));
				break;
			case 4:
				add(ds(v.byte_8B9D2[4] == 1 ? 0x1BF1 : 0x1C00));
				break;
			case 5:
				add(msgText(6));
				break;
			case 6:
				add(ds(0x1C10));
				break;
			}
		}
	}
	add(msgText(7));
	for (int i = firstWrong; i >= 0 && i < 7; i++) {
		if (guess[i] == recipe[i])
			continue;
		if (i != firstWrong)
			add(ds(0x1C1A));
		if (i == lastWrong && wrong > 1)
			add(ds(0x1C1D));
		switch (i) {
		case 0:
			add(ds(v.byte_8B9D2[0] == 0 ? 0x1C22 : 0x1C2D));
			add(ds(0x1C36));
			break;
		case 1:
			add(msgText(4));
			break;
		case 2:
			if (v.byte_8B9D2[2] == 0)
				add(ds(0x1C45));
			else if (v.byte_8B9D2[2] == 1)
				add(ds(0x1C51));
			else if (v.byte_8B9D2[2] == 2)
				add(ds(0x1C5B));
			add(ds(0x1C62));
			break;
		case 3:
			add(ds(0x1C6C));
			break;
		case 4:
			add(ds(v.byte_8B9D2[4] == 0 ? 0x1C73 : 0x1C83));
			break;
		case 5:
			add(msgText(8));
			break;
		case 6:
			add(ds(0x1C92));
			break;
		}
	}
	add(ds(0x1C9C));
	return msg;
}

} // namespace Xanth

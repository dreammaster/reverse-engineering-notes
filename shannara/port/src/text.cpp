#include "text.h"
#include <cctype>
#include <cstring>

namespace shannara {

static const size_t kLimit = 0x7E; // the original never writes past 126 characters

Text::Text(const LeExe &exe, const ObjectDat &names, World &world) : _names(names), _w(world) {
	_playerA = exe.u16(0xADEB0);
	_playerB = exe.u16(0xADEB2);
	_specialObject = exe.u16(0xAFF50); // object whose name is computed (sub_6ECF0: "Black Irix" once mentioned, else "dark object")
}

bool Text::hasName(int obj) const {
	if (obj == _specialObject)
		return true;
	return _w.valid(obj) && _w.state().objects[obj - 1].nameOffset() != 0;
}

std::string Text::name(int obj) const {
	if (obj == _specialObject)
		return _w.hasAttr(obj, 6) ? "Black Irix" : "dark object";
	if (!_w.valid(obj))
		return std::string();
	return _names.text(_w.state().objects[obj - 1].nameOffset());
}

bool Text::printObj(uint32_t flags, int obj, const char *verb, std::string &out, std::string *printed) {
	out.clear();
	if (!_w.valid(obj))
		return false;
	std::string buf;
	auto full = [&]() { return buf.size() >= kLimit; };
	const int form = flags & kFormMask;
	bool cap = false;

	if (form) {
		cap = (flags & kCapital) != 0;
		if (!cap)
			buf.push_back(' ');
		const char *word = nullptr;
		const bool me = isPlayer(obj);
		const bool person = _w.state().objects[obj - 1].type == 5;
		const bool female = person && _w.hasAttr(obj, 0x16);
		switch (form) {
		case 2:
			if (!_w.hasAttr(obj, 2))
				word = "the";
			break;
		case 3:
			if (!_w.hasAttr(obj, 2)) {
				if (_w.hasAttr(obj, 3) || _w.hasAttr(obj, 1))
					word = "some";
				else if (_w.hasAttr(obj, 4))
					word = "an";
				else
					word = "a";
			}
			break;
		case 4:
			word = me ? "you" : plural(obj) ? "they" : person ? (female ? "she" : "he") : "it";
			break;
		case 5:
			word = me ? "your" : plural(obj) ? "their" : person ? (female ? "her" : "his") : "its";
			break;
		case 6:
			word = me ? "you" : plural(obj) ? "them" : person ? (female ? "her" : "him") : "it";
			break;
		case 7:
			word = obj == _playerA ? "yourself" : plural(obj) ? "themselves" : person ? (female ? "herself" : "himself") : "itself";
			break;
		default:
			break; // form 1: no word
		}
		if (word) {
			bool first = true;
			for (const char *p = word; *p && (first || !full()); p++) {
				// the first character is copied unconditionally (and upper-cased when requested), as in the original
				buf.push_back(first && cap ? (char)toupper((unsigned char)*p) : *p);
				if (first)
					cap = false;
				first = false;
			}
			if ((form == 2 || form == 3) && !full())
				buf.push_back(' ');
		}
		if (form >= 1 && form <= 3) {
			if (me) {
				buf += cap ? "You" : "you";
			} else {
				std::string nm = hasName(obj) ? name(obj) : std::string();
				if (!nm.empty()) {
					size_t i = 0;
					if (cap && !full()) {
						buf.push_back((char)toupper((unsigned char)nm[0]));
						i = 1;
					}
					for (; i < nm.size() && !full(); i++)
						buf.push_back(nm[i]);
				}
			}
		}
	}

	if (flags & kVerb) {
		const char *v = verb;
		if (v && *v) {
			size_t len = strlen(v);
			if (!full())
				buf.push_back(' ');
			if (isPlayer(obj) || plural(obj)) {
				if (buf.size() + len < kLimit)
					buf += v;
			} else {
				static const char *const irregular[5][2] = {{"are", "is"}, {"were", "was"}, {"have", "has"}, {"do", "does"}, {"go", "goes"}};
				bool done = false;
				int forceIdx = -1; // regular-ending test is skipped (index 6) after the quirk below
				for (int k = 0; k < 5; k++) {
					size_t plen = strlen(irregular[k][0]);
					if (len < plen)
						continue;
					size_t diff = len - plen;
					if (strcmp(v + diff, irregular[k][0]) == 0 && (diff == 0 || v[diff - 1] == ' ')) {
						if (strlen(irregular[k][1]) + buf.size() + diff < kLimit) {
							buf.append(v, diff);
							buf += irregular[k][1];
							// quirk of the original: the loop counter that copied the `diff` prefix characters doubles as the "matched" test,
							// so a multi-word verb whose irregular word is preceded by 5 or more characters ("want to go") also falls into
							// the regular rule below and gets the whole verb appended again with an "s"
							if (diff >= 5) {
								forceIdx = 6;
								break;
							}
						}
						done = true;
						break;
					}
				}
				if (!done) {
					static const char *const endings[6] = {"ry", "ly", "ss", "zz", "sh", "ch"};
					int idx = 6;
					if (len > 2 && forceIdx < 0) {
						idx = 0;
						while (idx < 6 && strcmp(v + len - 2, endings[idx]) != 0)
							idx++;
					}
					if (buf.size() + len < kLimit) {
						if (idx < 2) {
							buf.append(v, len - 2);
							buf += "ies";
						} else {
							buf += v;
							if (idx < 6)
								buf.push_back('e');
							buf.push_back('s');
						}
					}
				}
			}
		}
	}

	if ((flags & kPeriod) && buf.size() + 2 < kLimit)
		buf.push_back('.');

	_w.setAttr(obj, 6);
	if (flags & kPrintNow) {
		if (printed)
			*printed = buf;
		return false;
	}
	out = buf;
	return true;
}

} // namespace shannara

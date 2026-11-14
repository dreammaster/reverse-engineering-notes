#include "ultima6/md_create.h"

#include "files/data_file.h"
#include "files/lib_n.h"
#include "files/world_data.h"

#include <algorithm>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/md_create_data.inc"

namespace {

const double TICKS_PER_SECOND = 18.2; // the BIOS clock, as the original divides it (dbl_27A9F)
const int FADE_FRAME_US = 14268;      // a retrace of the VGA

// The pictures of the *.LZC are archives inside an item: a table of u32 that start at 4 bytes less than they say, the shapes behind it
bool nestedShape(const Bytes &item, size_t k, Shape &out) {
	if (item.size() < 8)
		return false;
	auto u32 = [&](size_t o) { return (uint32_t)item[o] | ((uint32_t)item[o + 1] << 8) | ((uint32_t)item[o + 2] << 16) | ((uint32_t)item[o + 3] << 24); };
	size_t first = u32(0), n = first / 4 - 1;
	if (k >= n || first > item.size())
		return false;
	size_t a = u32(k * 4) - 4, e = k + 1 < n ? u32((k + 1) * 4) - 4 : item.size();
	return a < e && e <= item.size() && out.load(&item[a], e - a);
}

bool loadShapes(const std::string &dir, const char *file, size_t item, std::vector<Shape> &out) {
	Bytes raw, data;
	LibN lib;
	if (!readFile(joinPath(dir, file), raw) || !lib.load(raw, false) || !lib.get(item, data))
		return false;
	for (size_t k = 0;; ++k) {
		Shape s;
		if (!nestedShape(data, k, s))
			break;
		out.push_back(s);
	}
	return !out.empty();
}

int lower(int c) {
	return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

} // namespace

void MdNewCharacter::applyTo(ObjList &al) const {
	std::memset(al.partyNames[0], 0, sizeof al.partyNames[0]);
	std::memcpy(al.partyNames[0], name.c_str(), std::min<size_t>(name.size(), 13));
	al.strength[1] = (uint8_t)strength;
	al.dexterity[1] = (uint8_t)dexterity;
	al.intelligence[1] = (uint8_t)intelligence;
	al.level[1] = 4;
	al.exp[1] = 0x258;
	al.hp[1] = (uint8_t)(4 * 0x18 + strength * 2);
	al.moves[1] = 0x14;
	al.objFrame[1] = (uint16_t)((al.objFrame[1] & 0xFC00) | (female ? 0x158 : 0x157));
	al.baseObjFrame[0] = al.baseObjFrame[1] = al.objFrame[1];
}

MdCreation::MdCreation(const std::string &dir, uint32_t seed) : _dir(dir), _rng(seed) {
	std::memset(_font.glyph, 0, sizeof _font.glyph);
	Bytes iff;
	if (!readFile(joinPath(dir, "MD_TITLE.PAL"), iff))
		return;
	bool found = false;
	for (size_t i = 12; i + 8 <= iff.size() && !found;) {
		size_t size = ((size_t)iff[i + 4] << 24) | ((size_t)iff[i + 5] << 16) | ((size_t)iff[i + 6] << 8) | iff[i + 7];
		if (!std::memcmp(&iff[i], "CMAP", 4) && size >= 768 && i + 8 + 768 <= iff.size()) {
			for (int c = 0; c < 256; ++c)
				for (int k = 0; k < 3; ++k)
					_palette.rgb[c][k] = (uint8_t)(iff[i + 8 + c * 3 + k] >> 2);
			found = true;
		}
		i += 8 + size + (size & 1);
	}
	Bytes fontRaw, fontData;
	LibN fontLib;
	if (!found || !readFile(joinPath(dir, "FONTS.LZC"), fontRaw) || !fontLib.load(fontRaw, true) || !fontLib.get(0, fontData))
		return;
	_font.loadWou(fontData);
	// CREATE.LZC items 0-2 of the original (1-3 of the archive): the study with the faces, the foreground, the small figure
	if (!loadShapes(dir, "CREATE.LZC", 1, _face) || !loadShapes(dir, "CREATE.LZC", 2, _foreground) || !loadShapes(dir, "CREATE.LZC", 3, _figure))
		return;
	// SCENETXT.LZC item 5 (6 of the archive): the lines, cut at the carriage returns; line 0 is the piece before the first
	Bytes raw, item;
	LibN lib;
	if (!readFile(joinPath(dir, "SCENETXT.LZC"), raw) || !lib.load(raw, false) || !lib.get(6, item))
		return;
	std::string cur;
	for (uint8_t c : item) {
		if (c == 0x0D) {
			_lines.push_back(cur);
			cur.clear();
		} else if (c != 0x0A) {
			cur += (char)c;
		}
	}
	_lines.push_back(cur);
	_front.assign(320 * 200, 0);
	// sub_18557: the three random numbers, and every stat starts as the third
	_random[0] = random(0x1A, 0x18);
	_random[1] = random(0x18, 0x16);
	_random[2] = random(0x16, 0x12);
	_stat[0] = _stat[1] = _stat[2] = _random[2];
	_ok = true;
	runProgram();
}

// sub_1E59F with the generator of sub_1E582: the state is three times itself plus its low half shifted up; the result is the high half
int MdCreation::random(int a, int b) {
	uint32_t lo = _rng & 0xFFFF, hi = _rng >> 16;
	uint32_t nlo = (lo * 3) & 0xFFFF;
	uint32_t nhi = (hi * 3 + ((lo * 3) >> 16) + lo) & 0xFFFF;
	_rng = (nhi << 16) | nlo;
	int low = std::min(a, b), high = std::max(a, b);
	unsigned range = (unsigned)(high - low + 1) & 0xFFFF;
	return low + (range ? (int)(nhi % range) : 0);
}

uint32_t MdCreation::now() const {
	return (uint32_t)(_ms * TICKS_PER_SECOND / 1000.0);
}

void MdCreation::key(int k) {
	if (_state != RUN)
		return;
	if (k == KEY_ESC) {
		_state = CANCELLED;
		return;
	}
	if (k != KEY_NONE)
		_keys.push_back(k);
}

void MdCreation::tick(unsigned ms) {
	for (unsigned i = 0; i < ms && _state != DONE && _state != CANCELLED; ++i) {
		_ms += 1;
		if (_state == FADE) {
			_fadeLeft -= 1000;
			if (_fadeLeft <= 0)
				_state = DONE;
			continue;
		}
		pass();
	}
}

// the questions' program (sub_18557 between the first question and loc_19166)
void MdCreation::runProgram() {
	const size_t count = sizeof MD_CREATE_OPS / sizeof MD_CREATE_OPS[0];
	while (!_inScene && _state == RUN) {
		if (_final) { // the scene after the questions is over: the stats are put into the character
			_final = false;
			_result.female = _reg[1] == 'b';
			_result.strength = _stat[0];
			_result.dexterity = _stat[1];
			_result.intelligence = _stat[2];
			_state = FADE; // sub_155A5, the fade out, one step for each retrace: _fadeLeft is in microseconds
			int most = 0;
			for (int c = 0; c < 256; ++c)
				for (int k = 0; k < 3; ++k)
					most = std::max(most, (int)_palette.rgb[c][k]);
			_fadeLeft = most * FADE_FRAME_US;
			return;
		}
		if (_pc >= count)
			return;
		const MdCreateOp &o = MD_CREATE_OPS[_pc++];
		switch (o.op) {
		case 0:
			startScene(o.a);
			return;
		case 1:
			_reg[o.a] = _acc;
			break;
		case 2:
			_cmp = (o.a == 0 ? _acc : _reg[o.a]) == o.b;
			break;
		case 3:
			if (_cmp)
				_pc = (size_t)o.a;
			break;
		case 4:
			if (!_cmp)
				_pc = (size_t)o.a;
			break;
		case 5:
			_pc = (size_t)o.a;
			break;
		case 6:
			_stat[o.a] = _random[o.b];
			break;
		default: // 7: the last scene
			_final = true;
			startScene(0x45);
			return;
		}
	}
}

void MdCreation::startScene(int index) {
	_inScene = true;
	_sceneStart = _idx = index;
	_redraw = true;
	_value = -1;
	_t0 = _tMouth = _tBlink = _tFigure = now();
	_mouth = _blink = _dx = _dy = _frame = 0;
	_typing = false;
	_keys.clear();
}

void MdCreation::endScene(int value) {
	_inScene = false;
	_acc = value & 0xFFFF;
	if (_acc == 0xFFFF)
		_acc = -1;
}

void MdCreation::draw() {
	const int16_t *rec = MD_CREATE_SCENES[_idx];
	std::fill(_front.begin(), _front.end(), 0);
	auto put = [&](const std::vector<Shape> &list, int frame, int x, int y) {
		if (frame < 0 || frame >= (int)list.size())
			return;
		const Shape &s = list[frame];
		for (int r = 0; r < s.height; ++r) {
			int sy = y - s.hotY + r;
			if (sy < 0 || sy >= 200)
				continue;
			for (int c = 0; c < s.width; ++c) {
				int sx = x - s.hotX + c;
				uint8_t p = s.pixels[r * s.width + c];
				if (sx >= 0 && sx < 320 && p != Shape::TRANSPARENT)
					_front[sy * 320 + sx] = p;
			}
		}
	};
	put(_face, 0, 0, 24);
	put(_face, _mouth > 0 ? (rec[1] == 2 ? 5 : 4) : _blink > 0 ? 1 : rec[1], 0, 24);
	put(_figure, _frame, 0x80 - _dx, 0x53 + _dy);
	put(_foreground, 0, 0x59, 0x6D);
	// the line, in the colour the sign of its number says (4, or 6), over the black below the picture; "$P" is the name that was typed
	int n = rec[0] < 0 ? -rec[0] : rec[0];
	const std::string &line = n < (int)_lines.size() ? _lines[n] : std::string();
	std::string text;
	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == '$' && i + 1 < line.size() && lower((uint8_t)line[i + 1]) == 'p') {
			text += _result.name;
			++i;
		} else {
			text += line[i];
		}
	}
	_text = text;
	_textColour = rec[0] < 0 ? 4 : 6;
	std::vector<std::string> shown;
	std::string cur;
	for (char c : text) {
		if (c == '^') {
			shown.push_back(cur);
			cur.clear();
		} else {
			cur += c;
		}
	}
	shown.push_back(cur);
	// a line that is wider than the screen is broken at a space
	std::vector<std::string> out;
	for (const std::string &l : shown) {
		std::string row, word;
		int cw = 0, ww = 0;
		auto endWord = [&]() {
			if (word.empty())
				return;
			int space = row.empty() ? 0 : _font.width(' ');
			if (!row.empty() && cw + space + ww > 316) {
				out.push_back(row);
				row.clear();
				cw = 0;
				space = 0;
			}
			row += (row.empty() ? "" : " ") + word;
			cw += space + ww;
			word.clear();
			ww = 0;
		};
		for (char c : l) {
			if (c == ' ') {
				endWord();
			} else {
				word += c;
				ww += _font.width((uint8_t)c);
			}
		}
		endWord();
		out.push_back(row);
	}
	auto write = [&](int x, int y, const std::string &s, int colour) {
		Framebuffer tmp;
		tmp.clear(0);
		Ui::drawText(tmp, _font, x, y, s, 1);
		for (int i = 0; i < 320 * 200; ++i)
			if (tmp.pixels()[i])
				_front[i] = (uint8_t)colour;
	};
	int y = 160;
	for (const std::string &l : out) {
		int w = 0;
		for (char c : l)
			w += _font.width((uint8_t)c);
		write((320 - w) / 2, y, l, _textColour);
		y += 12;
	}
	if (_typing) { // the name, at x 120, y 184, in colour 14, with its cursor
		std::string s = _name;
		if (_name.size() < 12)
			s += _cursor ? '_' : ' ';
		write(120, 184, s, 14);
	}
}

// one turn of sub_17E7A's loop at the time now
void MdCreation::pass() {
	if (!_inScene) {
		runProgram();
		if (!_inScene)
			return;
	}
	const uint32_t n = now();
	const int16_t *rec = MD_CREATE_SCENES[_idx];
	if (_typing) { // the name: the original does not leave this loop until Enter
		uint32_t c = (n % 16) / 8;
		bool redraw = c != _cursor;
		_cursor = c;
		while (!_keys.empty()) {
			int k = _keys.front();
			_keys.pop_front();
			redraw = true;
			if (k == KEY_ENTER && !_name.empty()) {
				_result.name = _name;
				_typing = false;
				_keys.clear();
				_idx++;
				if (MD_CREATE_SCENES[_idx][0] != 0) {
					_redraw = true;
					_value = -1;
				} else {
					endScene(_value);
				}
				return;
			}
			if (k == KEY_BACKSPACE) {
				if (!_name.empty())
					_name.erase(_name.size() - 1);
			} else if (k < 256 && lower(k) >= 'a' && lower(k) <= 'z' && _name.size() < 12) {
				_name += (char)k;
			}
		}
		if (redraw)
			draw();
		return;
	}
	// animation (the faces blink and talk, the small figure drifts)
	if (_mouth > 0 && n > _tMouth + (uint32_t)_mouth) {
		_mouth = 0;
		_redraw = true;
	}
	if (_blink > 0 && n > _tBlink + (uint32_t)_blink) {
		_blink = 0;
		_redraw = true;
	}
	if (n > _tMouth + (uint32_t)random(100, 200)) {
		_tMouth = n;
		_mouth = random(2, 10);
		_redraw = true;
	}
	if (rec[2] != 0 && n > _tFigure + (uint32_t)random(2, 20) && _mouth == 0) {
		_tFigure = n;
		if (n > _tBlink + (uint32_t)random(150, 300)) {
			_tBlink = n;
			_blink = random(20, 80);
		}
		if (_blink == 0) {
			_dx += random(0, 3);
			if (_dx > 0x1D) {
				_dx %= 0x1E;
				_dy = (_dy + 2) % 0x14;
			}
			_frame = (int)((unsigned)(_frame + random(0, 2) - 1) & 0xFFFF) % 7;
		}
		_redraw = true;
	}
	if (_redraw)
		draw();
	bool keep = true; // var_20
	_redraw = false;
	_value = -1;
	const int mode = rec[3];
	if (mode >= 0) {
		if (!_keys.empty()) {
			keep = false;
			_keys.clear();
		} else if ((double)(n - _t0) / TICKS_PER_SECOND > mode) {
			keep = false;
		}
	} else if (mode == -1) {
		if (!_keys.empty()) {
			keep = false;
			_keys.clear();
		}
	} else if (mode == -2 || mode == -3) {
		if (!_keys.empty()) {
			int k = lower(_keys.front());
			_keys.pop_front();
			_value = k;
			if (k >= 'a' && k <= (mode == -2 ? 'b' : 'c')) {
				keep = false;
				_keys.clear();
			}
		}
	} else { // -4: the name
		_typing = true;
		_name.clear();
		_cursor = 0xFFFFFFFFu;
		_keys.clear();
		return;
	}
	if (keep)
		return;
	// the scene's next entry
	_idx++;
	if (MD_CREATE_SCENES[_idx][0] != 0) {
		_redraw = true;
		return;
	}
	endScene(_value);
}

void MdCreation::render(Framebuffer &screen) const {
	uint8_t pal[256][3];
	int most = 0;
	for (int c = 0; c < 256; ++c)
		for (int k = 0; k < 3; ++k)
			most = std::max(most, (int)_palette.rgb[c][k]);
	int step = 0;
	if (_state == FADE && most > 0)
		step = std::min(most, (int)(((long long)most * FADE_FRAME_US - _fadeLeft) / FADE_FRAME_US));
	for (int c = 0; c < 256; ++c)
		for (int k = 0; k < 3; ++k) {
			int v = _palette.rgb[c][k];
			if (_state == FADE) { // as the fades of the introduction (sub_1FF0B)
				int acc = most / 2, moved = 0;
				for (int t = 0; t < step; ++t) {
					acc += v;
					if (acc > most) {
						acc -= most;
						++moved;
					}
				}
				v -= moved;
			}
			pal[c][k] = (uint8_t)v;
		}
	screen.setPalette(pal);
	screen.clear(0);
	screen.blit(0, 0, &_front[0], 320, 200, 320);
}

} // namespace Ultima6
} // namespace Ultima6

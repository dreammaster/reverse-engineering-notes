#include "ultima6/md_intro.h"

#include "files/data_file.h"
#include "files/lib_n.h"
#include "ultima6/ui.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/md_intro_script.inc"

namespace {

const int WINDOW_TOP = 24, WINDOW_BOTTOM = 152; // the picture is 128 lines high, 24 down
const int FADE_MS = 500;

// The pictures of the *.LZC are archives inside the item of an archive: a table of u32 that start at 4 bytes less than they say, the shapes behind it
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

} // namespace

MdIntro::MdIntro(const std::string &dir) : _dir(dir) {
	std::memset(_font.glyph, 0, sizeof _font.glyph);
	// the colours: the CMAP chunk of the IFF picture MD_TITLE.PAL (8 bits a colour, the screen takes 6)
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
	if (!found)
		return;
	Bytes fontRaw, fontData;
	LibN fontLib;
	if (readFile(joinPath(dir, "FONTS.LZC"), fontRaw) && fontLib.load(fontRaw, true) && fontLib.get(0, fontData))
		_font.loadWou(fontData);
	_front.assign(320 * 200, 0);
	_handles.resize(128);
	_texts.resize(8);
	_ok = true;
	run();
}

// the shapes of the archive of a loaded picture (item n of the original is item n + 1 of the archive, whose first entry is its size)
const std::vector<Shape> &MdIntro::shapes(int handle) {
	Handle &h = _handles[handle];
	if (!h.loaded) {
		h.loaded = true;
		Bytes raw, item;
		LibN lib;
		if (h.file >= 0 && readFile(joinPath(_dir, MD_INTRO_FILES[h.file]), raw) && lib.load(raw, false) && lib.get((size_t)h.item + 1, item)) {
			for (size_t k = 0;; ++k) {
				Shape s;
				if (!nestedShape(item, k, s))
					break;
				h.shapes.push_back(s);
			}
		}
	}
	return h.shapes;
}

// a line of SCENETXT.LZC: the lines of an item are cut at the carriage returns, the first piece (before the first) is line 0
const std::string &MdIntro::line(int set, int index) {
	static const std::string empty;
	if (set < 0 || set >= (int)_texts.size())
		return empty;
	std::vector<std::string> &t = _texts[set];
	if (t.empty()) {
		Bytes raw, item;
		LibN lib;
		if (readFile(joinPath(_dir, "SCENETXT.LZC"), raw) && lib.load(raw, false) && lib.get((size_t)set + 1, item)) {
			std::string cur;
			for (uint8_t c : item) {
				if (c == 0x0D) {
					t.push_back(cur);
					cur.clear();
				} else if (c != 0x0A) { // (a line feed follows each carriage return)
					cur += (char)c;
				}
			}
			t.push_back(cur);
		}
		if (t.empty())
			t.push_back("");
	}
	return index >= 0 && index < (int)t.size() ? t[index] : empty;
}

void MdIntro::shapeAt(int handle, int frame, int x, int y, int dy) {
	const std::vector<Shape> &list = shapes(handle);
	if (frame < 0 || frame >= (int)list.size())
		return;
	const Shape &s = list[frame];
	int left = x - s.hotX, top = y - s.hotY + dy;
	for (int r = 0; r < s.height; ++r) {
		int sy = top + r;
		if (sy < WINDOW_TOP || sy >= WINDOW_BOTTOM)
			continue;
		for (int c = 0; c < s.width; ++c) {
			int sx = left + c;
			uint8_t p = s.pixels[r * s.width + c];
			if (sx < 0 || sx >= 320 || p == Shape::TRANSPARENT)
				continue;
			_front[sy * 320 + sx] = p;
		}
	}
}

void MdIntro::compose() {
	std::fill(_front.begin(), _front.end(), 0);
	for (const Draw &d : _draws) {
		if (d.kind == 0) {
			shapeAt(d.handle, d.frame, d.x, d.y);
		} else if (d.kind == 1) { // the hand with the map moves up and to the left over the window
			shapeAt(d.handle, d.frame, d.x, WINDOW_TOP, d.y);
		} else if (d.y >= WINDOW_TOP && d.y < WINDOW_BOTTOM && d.x >= 0 && d.x < 320) { // a dot of the orb
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
					if (d.y + dy >= WINDOW_TOP && d.y + dy < WINDOW_BOTTOM && d.x + dx >= 0 && d.x + dx < 320)
						_front[(d.y + dy) * 320 + d.x + dx] = 0xFF;
		}
	}
	// a line that is wider than the window is broken at a space
	std::vector<std::string> out;
	for (const std::string &l : _lines) {
		std::string cur, word;
		int cw = 0, ww = 0;
		auto endWord = [&]() {
			if (word.empty())
				return;
			int space = cur.empty() ? 0 : _font.width(' ');
			if (!cur.empty() && cw + space + ww > 316) {
				out.push_back(cur);
				cur.clear();
				cw = 0;
				space = 0;
			}
			cur += (cur.empty() ? "" : " ") + word;
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
		out.push_back(cur);
	}
	int y = _captionY;
	for (const std::string &l : out) {
		int w = 0;
		for (char c : l)
			w += _font.width((uint8_t)c);
		// the writing is done straight into the picture
		Framebuffer tmp;
		tmp.clear(0);
		Ui::drawText(tmp, _font, (320 - w) / 2, y, l, 1);
		for (int i = 0; i < 320 * 200; ++i)
			if (tmp.pixels()[i])
				_front[i] = 0xDD; // APPROX(90): the colour of the lines
		y += 12;
	}
}

void MdIntro::run() {
	const size_t count = sizeof MD_INTRO_OPS / sizeof MD_INTRO_OPS[0];
	while (_pc < count && _wait <= 0 && _fade == 0 && !_finished) {
		const MdIntroOp &o = MD_INTRO_OPS[_pc++];
		switch (o.op) {
		case 0:
			if (o.a >= 0 && o.a < (int)_handles.size()) {
				_handles[o.a] = Handle();
				_handles[o.a].file = o.b;
				_handles[o.a].item = o.c;
			}
			break;
		case 1:
			break;
		case 2:
			_draws.clear();
			_lines.clear();
			break;
		case 3:
			_draws.push_back({0, o.a, o.b, o.c, o.d});
			break;
		case 4: {
			_lines.clear();
			const std::string &t = line(o.a, o.b);
			std::string cur;
			for (char c : t) {
				if (c == '^') {
					_lines.push_back(cur);
					cur.clear();
				} else {
					cur += c;
				}
			}
			_lines.push_back(cur);
			_captionY = o.c;
			break;
		}
		case 5:
			compose();
			_black = false;
			break;
		case 6:
			_wait = o.a;
			break;
		case 7:
			compose();
			_black = false;
			_fade = 1;
			_fadeLeft = FADE_MS;
			break;
		case 8:
			_fade = -1;
			_fadeLeft = FADE_MS;
			break;
		case 9:
			_draws.push_back({1, o.a, o.b, o.c, o.d});
			break;
		case 10:
			_draws.push_back({2, 0, 0, o.a, o.b});
			break;
		}
	}
	if (_pc >= count && _wait <= 0 && _fade == 0)
		_finished = true;
}

void MdIntro::tick(unsigned ms) {
	int left = (int)ms;
	while (left > 0 && !_finished) {
		if (_wait > 0) {
			int step = std::min(_wait, left);
			_wait -= step;
			left -= step;
		} else if (_fade != 0) {
			int step = std::min(_fadeLeft, left);
			_fadeLeft -= step;
			left -= step;
			if (_fadeLeft <= 0) {
				if (_fade < 0)
					_black = true;
				_fade = 0;
			}
		} else {
			left = 0;
		}
		run();
	}
	if (_wait <= 0 && _fade == 0)
		run();
}

void MdIntro::runToEnd() {
	while (!_finished)
		tick(1000);
}

void MdIntro::render(Framebuffer &screen) const {
	uint8_t pal[256][3];
	int level = _black ? 0 : 1000;
	if (_fade > 0)
		level = 1000 * (FADE_MS - _fadeLeft) / FADE_MS;
	else if (_fade < 0)
		level = 1000 * _fadeLeft / FADE_MS;
	for (int c = 0; c < 256; ++c)
		for (int k = 0; k < 3; ++k)
			pal[c][k] = (uint8_t)(_palette.rgb[c][k] * level / 1000);
	screen.setPalette(pal);
	screen.clear(0);
	screen.blit(0, 0, &_front[0], 320, 200, 320);
}

} // namespace Ultima6
} // namespace Ultima6

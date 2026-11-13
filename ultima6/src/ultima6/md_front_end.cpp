#include "ultima6/md_front_end.h"

#include "files/data_file.h"
#include "files/lib_n.h"
#include "ultima6/ui.h"

#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

// The pictures of the *.LZC of Martian Dreams are archives inside the item of an archive: a table of u32 that start at 4 bytes less than they say, the shapes behind it
bool MdFrontEnd::nested(const Bytes &item, size_t k, Shape &out) const {
	if (item.size() < 8)
		return false;
	auto u32 = [&](size_t o) { return (uint32_t)item[o] | ((uint32_t)item[o + 1] << 8) | ((uint32_t)item[o + 2] << 16) | ((uint32_t)item[o + 3] << 24); };
	size_t first = u32(0), n = first / 4 - 1;
	if (k >= n || first > item.size())
		return false;
	size_t a = u32(k * 4) - 4, e = k + 1 < n ? u32((k + 1) * 4) - 4 : item.size();
	return a < e && e <= item.size() && out.load(&item[a], e - a);
}

MdFrontEnd::MdFrontEnd(const std::string &dir) : _dir(dir) {
	std::memset(_palette, 0, sizeof _palette);
	std::memset(_font.glyph, 0, sizeof _font.glyph);
	Bytes raw, item;
	LibN lib;
	if (!readFile(joinPath(dir, "MDMENU.LZC"), raw) || !lib.load(raw, false) || !lib.get(1, item))
		return;
	if (!nested(item, 0, _picture) || !nested(item, 1, _frameOff) || !nested(item, 2, _frame) || _picture.width != 320 || _picture.height != 200)
		return;
	// the colours: the CMAP chunk of the IFF picture MD_TITLE.PAL (8 bits a colour, the screen takes 6)
	Bytes iff;
	if (!readFile(joinPath(dir, "MD_TITLE.PAL"), iff))
		return;
	for (size_t i = 12; i + 8 <= iff.size();) {
		size_t size = ((size_t)iff[i + 4] << 24) | ((size_t)iff[i + 5] << 16) | ((size_t)iff[i + 6] << 8) | iff[i + 7];
		if (!std::memcmp(&iff[i], "CMAP", 4) && size >= 768 && i + 8 + 768 <= iff.size()) {
			for (int c = 0; c < 256; ++c)
				for (int k = 0; k < 3; ++k)
					_palette[c][k] = (uint8_t)(iff[i + 8 + c * 3 + k] >> 2);
			_ok = true;
			break;
		}
		i += 8 + size + (size & 1);
	}
	Bytes fontRaw, fontData;
	LibN fontLib;
	if (readFile(joinPath(dir, "FONTS.LZC"), fontRaw) && fontLib.load(fontRaw, true) && fontLib.get(0, fontData))
		_font.loadWou(fontData);
}

// The lines the introduction says (SCENETXT.LZC, items 1-3 for the three scenes) are separated by 0x0D and a '^' breaks one inside; the backdrops of a scene are its 320 x 128 pictures
bool MdFrontEnd::loadIntro() {
	if (!_pages.empty())
		return true;
	Bytes raw, palRaw, textRaw;
	LibN textLib;
	if (!readFile(joinPath(_dir, "MDPAL"), palRaw) || !_gamePalette.load(palRaw) || !readFile(joinPath(_dir, "SCENETXT.LZC"), textRaw) || !textLib.load(textRaw, false))
		return false;
	static const char *const FILES[3] = {"SCENE1.LZC", "SCENE2.LZC", "SCENE3.LZC"};
	for (int scene = 0; scene < 3; ++scene) {
		LibN lib;
		Bytes sraw, item, text;
		if (!readFile(joinPath(_dir, FILES[scene]), sraw) || !lib.load(sraw, false) || !textLib.get((size_t)scene + 1, text))
			return false;
		std::vector<size_t> backdrops;
		for (size_t i = 1; i < lib.count(); ++i) {
			if (!lib.present(i) || !lib.get(i, item))
				continue;
			for (size_t k = 0; k < 4; ++k) {
				Shape s;
				if (nested(item, k, s) && s.width == 320 && s.height == 128) {
					_pictures.push_back(s);
					backdrops.push_back(_pictures.size() - 1);
				}
			}
		}
		std::vector<std::string> captions;
		std::string line;
		for (uint8_t c : text) {
			if (c == 0x0D) {
				if (!line.empty())
					captions.push_back(line);
				line.clear();
			} else {
				line += c == '^' ? '\n' : (char)c;
			}
		}
		if (!line.empty())
			captions.push_back(line);
		if (backdrops.empty())
			continue;
		for (size_t i = 0; i < captions.size(); ++i) {
			Page p;
			p.picture = backdrops[i * backdrops.size() / captions.size()];
			p.text = captions[i];
			_pages.push_back(p);
		}
	}
	return !_pages.empty();
}

// the text broken into lines of at most 300 pixels, at the spaces and at the newlines that stand for the original's '^'
void MdFrontEnd::wrap(const std::string &text, std::vector<std::string> &lines) const {
	std::string cur, word;
	int width = 0, wordWidth = 0;
	auto endWord = [&]() {
		if (word.empty())
			return;
		int space = cur.empty() ? 0 : _font.width(' ');
		if (!cur.empty() && width + space + wordWidth > 300) {
			lines.push_back(cur);
			cur.clear();
			width = 0;
			space = 0;
		}
		cur += (cur.empty() ? "" : " ") + word;
		width += space + wordWidth;
		word.clear();
		wordWidth = 0;
	};
	for (char c : text) {
		if (c == '\n') {
			endWord();
			lines.push_back(cur);
			cur.clear();
			width = 0;
		} else if (c == ' ') {
			endWord();
		} else {
			word += c;
			wordWidth += _font.width((uint8_t)c);
		}
	}
	endWord();
	if (!cur.empty())
		lines.push_back(cur);
}

void MdFrontEnd::render(Framebuffer &screen) {
	if (_state == INTRO && _page < _pages.size()) {
		screen.setPalette(_gamePalette.rgb);
		screen.clear(0);
		const Shape &s = _pictures[_pages[_page].picture];
		screen.blit(0, 0, &s.pixels[0], 320, 128, 320);
		std::vector<std::string> lines;
		wrap(_pages[_page].text, lines);
		int y = 134;
		for (const std::string &l : lines) {
			int w = 0;
			for (char c : l)
				w += _font.width((uint8_t)c);
			Ui::drawText(screen, _font, (320 - w) / 2, y, l, 0xE6); // APPROX(89): the colour of the caption
			y += _font.height() + 1;
		}
		return;
	}
	screen.setPalette(_palette);
	screen.clear(0);
	screen.blit(0, 0, &_picture.pixels[0], 320, 200, 320);
	if (_state == MENU || _state == NAME) {
		int y = 130 + 17 * _selected;
		screen.blit(27, y, &_frameOff.pixels[0], _frameOff.width, _frameOff.height, _frameOff.width, 255);
		screen.blit(27, y, &_frame.pixels[0], _frame.width, _frame.height, _frame.width, 255);
	}
	if (_state == NAME || !_message.empty()) {
		screen.fillRect(20, 96, 280, 22, 0);
		std::string line = _state == NAME ? "Name: " + _name + "_" : _message;
		Ui::drawText(screen, _font, 26, 102, line, 221);
	}
}

bool MdFrontEnd::handleKey(int key) {
	if (key == KEY_QUIT) {
		_state = QUIT;
		return false;
	}
	if (_state == INTRO) {
		if (key == KEY_ESC || ++_page >= _pages.size())
			_state = MENU;
		return true;
	}
	if (_state == NAME) {
		if (key == KEY_ESC) {
			_name.clear();
			_state = MENU;
		} else if (key == KEY_ENTER) {
			if (!_name.empty()) {
				_state = DONE;
				return false;
			}
		} else if (key == KEY_BACKSPACE) {
			if (!_name.empty())
				_name.erase(_name.size() - 1);
		} else if (key >= 32 && key < 127 && _name.size() < 13) {
			_name += (char)key;
		}
		return true;
	}
	_message.clear();
	int choice = -1;
	if (key == KEY_UP)
		_selected = (_selected + CHOICES - 1) % CHOICES;
	else if (key == KEY_DOWN)
		_selected = (_selected + 1) % CHOICES;
	else if (key >= '1' && key <= '4')
		choice = _selected = key - '1';
	else if (key == KEY_ENTER || key == KEY_SPACE)
		choice = _selected;
	else if (key == KEY_ESC) {
		_state = QUIT;
		return false;
	}
	switch (choice) {
	case 0:
		if (loadIntro()) {
			_page = 0;
			_state = INTRO;
		} else {
			_message = "The introduction's files are missing.";
		}
		break;
	case 1:
		_state = NAME;
		_name.clear();
		break;
	case 2:
		_state = DONE;
		return false;
	case 3:
		_message = "MARTIAN DREAMS (c) 1991 Origin Systems";
		break;
	default:
		break;
	}
	return true;
}

} // namespace Ultima6
} // namespace Ultima6

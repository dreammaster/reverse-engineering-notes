#include "ultima6/md_front_end.h"

#include "files/data_file.h"
#include "files/lib_n.h"
#include "ultima6/ui.h"

#include <cstring>
#include <ctime>

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

void MdFrontEnd::tick(unsigned ms) {
	if (_state == INTRO && _intro) {
		_intro->tick(ms);
		if (_intro->finished())
			_state = MENU;
	} else if (_state == CREATE && _create) {
		_create->tick(ms);
		if (_create->finished()) {
			// the original writes the new savegame and is back in the menu: "Continue Game" plays it
			_character = _create->result();
			_hasCharacter = true;
			_asked = true;
			_state = MENU;
		} else if (_create->cancelled()) {
			_state = MENU;
		}
	}
}

void MdFrontEnd::render(Framebuffer &screen) {
	if (_state == INTRO && _intro) {
		_intro->render(screen);
		return;
	}
	if (_state == CREATE && _create) {
		_create->render(screen);
		return;
	}
	screen.setPalette(_palette);
	screen.clear(0);
	screen.blit(0, 0, &_picture.pixels[0], 320, 200, 320);
	if (_state == MENU) {
		int y = 130 + 17 * _selected;
		screen.blit(27, y, &_frameOff.pixels[0], _frameOff.width, _frameOff.height, _frameOff.width, 255);
		screen.blit(27, y, &_frame.pixels[0], _frame.width, _frame.height, _frame.width, 255);
	}
	if (!_message.empty()) {
		screen.fillRect(20, 96, 280, 22, 0);
		Ui::drawText(screen, _font, 26, 102, _message, 221);
	}
}

bool MdFrontEnd::handleKey(int key) {
	if (key == KEY_QUIT) {
		_state = QUIT;
		return false;
	}
	if (_state == INTRO) {
		_intro->stop(); // any key ends it, as in the original
		_state = MENU;
		return true;
	}
	if (_state == CREATE) { // the keys of the scenes (Escape leaves, as nothing in the original does)
		_create->key(key);
		if (_create->cancelled())
			_state = MENU;
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
		_intro.reset(new MdIntro(_dir));
		if (_intro->ok()) {
			_state = INTRO;
		} else {
			_message = "The introduction's files are missing.";
		}
		break;
	case 1: // Dr. Freud's questions; a character that was made stays (the original's byte_27E97)
		if (_asked) {
			_message = "Run INSTALL to remove your existing character before creating another.";
		} else {
			_create.reset(new MdCreation(_dir, (uint32_t)std::time(nullptr)));
			if (_create->ok()) {
				_state = CREATE;
			} else {
				_message = "The creation's files are missing.";
			}
		}
		break;
	case 2:
		_state = DONE;
		return false;
	case 3: // the credits: the logos, the fair, the people, the flight to Mars
		_intro.reset(new MdIntro(_dir, MdIntro::CREDITS));
		if (_intro->ok())
			_state = INTRO;
		else
			_message = "The credits' files are missing.";
		break;
	default:
		break;
	}
	return true;
}

} // namespace Ultima6
} // namespace Ultima6

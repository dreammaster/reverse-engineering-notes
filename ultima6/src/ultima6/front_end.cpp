#include "ultima6/front_end.h"

#include <cctype>
#include <cstdio>
#include <ctime>

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/gypsy_text.inc"

static const char *const MENU_ITEMS[5] = {"Introduction", "Create a Character", "Transfer a Character", "Acknowledgements", "Journey Onward"};
static const int MENU_ROW[5] = {16, 35, 57, 76, 97}; // centre of each item inside MAINMENU.SHP

static bool loadShapes(const std::string &dir, const char *name, std::vector<Shape> &out) {
	Bytes d;
	return readFile(joinPath(dir, name), d) && loadShapeFile(d, out);
}

FrontEnd::FrontEnd(const std::string &dir) : _dir(dir), _rng((uint16_t)std::time(nullptr)) {
	Bytes d;
	if (!readFile(joinPath(dir, "U6.CH"), d) || !_font.load(d))
		return;
	if (!readFile(joinPath(dir, "PALETTES.INT"), d) || !_palettes.load(d))
		return;
	if (!readFile(joinPath(dir, "U6PAL"), d) || !_gamePalette.load(d))
		return;
	if (!loadShapes(dir, "TITLES.SHP", _titles) || !loadShapes(dir, "MAINMENU.SHP", _menu) || !loadShapes(dir, "GYPSY.SHP", _gypsy) ||
		!loadShapes(dir, "VELLUM1.SHP", _vellum) || !loadShapes(dir, "MONTAGE.SHP", _montage))
		return;
	_hasSavedGame = readFile(joinPath(joinPath(dir, "SAVEGAME"), "OBJLIST"), d);
	_ok = true;
}

void FrontEnd::setPalette(Framebuffer &screen, int n) const {
	screen.setPalette(_palettes.palette[n].rgb);
}

int FrontEnd::brightest(int pal) const {
	int best = 1, score = -1;
	for (int i = 1; i < 255; ++i) {
		int s = _palettes.palette[pal].rgb[i][0] + _palettes.palette[pal].rgb[i][1] + _palettes.palette[pal].rgb[i][2];
		if (s > score) {
			score = s;
			best = i;
		}
	}
	return best;
}

int FrontEnd::darkest(int) const {
	return 0;
}

void FrontEnd::blitShape(Framebuffer &screen, const Shape &s, int x, int y) const {
	if (s.width > 0 && s.height > 0)
		screen.blit(x - s.hotX, y - s.hotY, &s.pixels[0], s.width, s.height, s.width, Shape::TRANSPARENT);
}

void FrontEnd::text(Framebuffer &screen, int x, int y, const std::string &s, int colour) const {
	for (size_t k = 0; k < s.size(); ++k) {
		const uint8_t *g = _font.glyph[(uint8_t)s[k]];
		for (int r = 0; r < 8; ++r)
			for (int b = 0; b < 8; ++b) {
				int px = x + (int)k * 8 + b, py = y + r;
				if ((g[r] & (0x80 >> b)) && px >= 0 && py >= 0 && px < Framebuffer::WIDTH && py < Framebuffer::HEIGHT)
					screen.pixels()[py * Framebuffer::WIDTH + px] = (uint8_t)colour;
			}
	}
}

void FrontEnd::wrapped(Framebuffer &screen, int x, int y, int cols, const std::string &s, int colour) const {
	size_t pos = 0;
	while (pos < s.size()) {
		size_t end = pos + cols;
		if (end >= s.size()) {
			end = s.size();
		} else {
			size_t sp = s.rfind(' ', end);
			if (sp != std::string::npos && sp > pos)
				end = sp;
		}
		text(screen, x, y, s.substr(pos, end - pos), colour);
		y += 8;
		pos = end;
		while (pos < s.size() && s[pos] == ' ')
			++pos;
	}
}

void FrontEnd::showMessage(const std::string &m) {
	_message = m;
	_after = _state;
	_state = MESSAGE;
}

// APPROX(38): not the original's exact behaviour, see docs/approximations.md
void FrontEnd::render(Framebuffer &screen) {
	screen.clear(0);
	switch (_state) {
	case MENU: {
		setPalette(screen, 0);
		blitShape(screen, _titles[0], 18, 2);
		blitShape(screen, _titles[1], 57, 54);
		blitShape(screen, _menu[0], 48, 84);
		text(screen, 54, 84 + MENU_ROW[_selected] - 4, ">", brightest(0));
		break;
	}
	case NAME:
	case SEX:
	case PORTRAIT: {
		screen.setPalette(_state == PORTRAIT ? _gamePalette.rgb : _palettes.palette[4].rgb);
		int c = brightest(4);
		if (_state == PORTRAIT) {
			int score = -1;
			for (int i = 1; i < 255; ++i) {
				int sum = _gamePalette.rgb[i][0] + _gamePalette.rgb[i][1] + _gamePalette.rgb[i][2];
				if (sum > score) {
					score = sum;
					c = i;
				}
			}
		}
		if (_state == NAME) {
			text(screen, 60, 80, "What is thy name?", c);
			text(screen, 60, 96, _name + "_", c);
		} else if (_state == SEX) {
			text(screen, 60, 80, _name + ", art thou", c);
			text(screen, 60, 96, "M)ale or F)emale?", c);
		} else {
			text(screen, 60, 16, "Choose thy likeness", c);
			int shape = (_character.female ? 0 : 6) + _portrait;
			blitShape(screen, _montage[shape], 132, 50);
			text(screen, 60, 150, "Left/Right to choose, Enter", c);
		}
		break;
	}
	case QUIZ:
	case RESULT: {
		setPalette(screen, 6);
		blitShape(screen, _gypsy[0], 0, 0);
		blitShape(screen, _vellum[0], 18, 82);
		int c = darkest(6);
		if (_state == QUIZ && _quiz) {
			int q = _quiz->question();
			wrapped(screen, 34, 92, 33, QUESTIONS[q - GypsyQuiz::FIRST_QUESTION], c);
			text(screen, 34, 184, "Press A or B", c);
		} else {
			char buf[80];
			text(screen, 34, 96, "Thy destiny is cast, " + _name + ".", c);
			std::snprintf(buf, sizeof buf, "Strength %d  Dexterity %d", _character.stats.strength, _character.stats.dexterity);
			text(screen, 34, 120, buf, c);
			std::snprintf(buf, sizeof buf, "Intelligence %d", _character.stats.intelligence);
			text(screen, 34, 132, buf, c);
			text(screen, 34, 184, "Press a key to begin", c);
		}
		break;
	}
	case MESSAGE: {
		setPalette(screen, 0);
		wrapped(screen, 40, 80, 30, _message, brightest(0));
		text(screen, 40, 170, "Press a key", brightest(0));
		break;
	}
	default:
		break;
	}
}

void FrontEnd::startCreation() {
	_name.clear();
	_character = NewCharacter();
	_created = false;
	_portrait = 0;
	_state = NAME;
}

void FrontEnd::answer(bool second) {
	_quiz->answer(second);
	if (_quiz->done()) {
		_character.stats = _quiz->stats();
		_character.name = _name;
		_created = true;
		_state = RESULT;
	}
}

bool FrontEnd::handleKey(int key) {
	if (key == KEY_QUIT) {
		_state = QUIT;
		return false;
	}
	switch (_state) {
	case MENU:
		if (key == KEY_UP)
			_selected = (_selected + 4) % 5;
		else if (key == KEY_DOWN)
			_selected = (_selected + 1) % 5;
		else if (key == KEY_ENTER || key == KEY_SPACE) {
			switch (_selected) {
			case 0:
				showMessage("The introduction is not yet part of this version.");
				break;
			case 1:
				startCreation();
				break;
			case 2:
				showMessage("Characters cannot yet be transferred from Ultima V or IV.");
				break;
			case 3:
				showMessage("Ultima VI: The False Prophet. Written by Richard Garriott, Origin Systems, Inc.");
				break;
			default:
				if (_created || _hasSavedGame) {
					_state = DONE;
					return false;
				}
				showMessage("You must first create or transfer a character.");
				break;
			}
		} else if (key == KEY_ESC) {
			_state = QUIT;
			return false;
		}
		break;
	case MESSAGE:
		_state = _after;
		break;
	case NAME:
		if (key == KEY_ESC)
			_state = MENU;
		else if (key == KEY_ENTER && !_name.empty())
			_state = SEX;
		else if (key == KEY_BACKSPACE && !_name.empty())
			_name.erase(_name.size() - 1);
		else if (key >= 32 && key < 127 && (int)_name.size() < NewCharacter::MAX_NAME)
			_name += (char)key;
		break;
	case SEX:
		if (key == KEY_ESC)
			_state = NAME;
		else if (key == 'm' || key == 'M' || key == 'f' || key == 'F') {
			_character.female = key == 'f' || key == 'F';
			_portrait = 0;
			_state = PORTRAIT;
		}
		break;
	case PORTRAIT:
		if (key == KEY_LEFT)
			_portrait = (_portrait + 5) % 6;
		else if (key == KEY_RIGHT)
			_portrait = (_portrait + 1) % 6;
		else if (key == KEY_ENTER) {
			_character.portrait = (_character.female ? 0 : 6) + _portrait;
			_quiz.reset(new GypsyQuiz(_rng));
			_state = QUIZ;
		} else if (key == KEY_ESC)
			_state = SEX;
		break;
	case QUIZ:
		if (key == 'a' || key == 'A')
			answer(false);
		else if (key == 'b' || key == 'B')
			answer(true);
		break;
	case RESULT:
		_state = MENU;
		_selected = 4; // Journey Onward
		break;
	default:
		break;
	}
	return _state != DONE && _state != QUIT;
}

} // namespace Ultima6
} // namespace Ultima6

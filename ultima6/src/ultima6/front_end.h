#ifndef ULTIMA6_ULTIMA6_FRONT_END_H
#define ULTIMA6_ULTIMA6_FRONT_END_H

#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "engine/random.h"
#include "files/shape.h"
#include "files/tile_data.h"
#include "ultima6/gypsy.h"

#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * What U.EXE does before the game: the main menu (MAINMENU.SHP over the TITLES logo, palette 0 of PALETTES.INT) and character
 * creation (create_character, 152C9): the name, the sex, one of six portraits from MONTAGE.SHP, and the gypsy's seven
 * questions (GYPSY.SHP with palette 6, the question on VELLUM1.SHP). The questions, answers and statistics come from
 * GypsyQuiz, which follows U.EXE exactly. The animated introduction, the transfer of characters from Ultima V / IV and the
 * acknowledgements pages are not done.
 *
 * Drawing positions are chosen to look like the original's; the original's own coordinates have not been traced.
 */
class FrontEnd {
public:
	enum State { MENU, NAME, SEX, PORTRAIT, QUIZ, RESULT, MESSAGE, INTRO, DONE, QUIT };

	explicit FrontEnd(const std::string &dir);

	bool ok() const {
		return _ok;
	}
	State state() const {
		return _state;
	}
	void render(Framebuffer &screen);
	/** Feeds a key; false when the front end is finished (see wantsGame()). */
	bool handleKey(int key);

	/** True when the player chose to start (a new character or the saved game). */
	bool wantsGame() const {
		return _state == DONE;
	}
	/** The created character, if any (otherwise the saved game is to be played). */
	bool hasNewCharacter() const {
		return _created;
	}
	const NewCharacter &character() const {
		return _character;
	}

private:
	void startCreation();
	void setPalette(Framebuffer &screen, int n) const;
	void blitShape(Framebuffer &screen, const Shape &s, int x, int y) const;
	void text(Framebuffer &screen, int x, int y, const std::string &s, int colour) const;
	void wrapped(Framebuffer &screen, int x, int y, int cols, const std::string &s, int colour) const;
	int brightest(int pal) const;
	int darkest(int pal) const;
	void showMessage(const std::string &m);
	void answer(bool second);

	bool _ok = false;
	State _state = MENU;
	State _after = MENU;
	std::string _message;
	int _selected = 1;
	std::string _dir;
	Font _font;
	IntroPalettes _palettes;
	Palette _gamePalette; // U6PAL, the palette the portraits are painted for
	std::vector<Shape> _titles, _menu, _gypsy, _vellum, _montage;
	std::vector<Shape> _intro[3]; // INTRO_1.SHP, INTRO_2.SHP, INTRO_3.SHP
	bool _hasIntro = false;
	int _introPage = 0;
	BorlandRandom _rng;
	std::unique_ptr<GypsyQuiz> _quiz;
	NewCharacter _character;
	bool _created = false;
	bool _hasSavedGame = false;
	int _portrait = 0;
	std::string _name;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

#ifndef ULTIMA6_ULTIMA6_MD_FRONT_END_H
#define ULTIMA6_ULTIMA6_MD_FRONT_END_H

#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * What MARTIAN.EXE shows first: the menu of MDMENU.LZC (item 1: a 320 x 200 picture of the title and the four choices, and a frame of 266 x 19 that is put round the one chosen; the palette
 * is the CMAP of MD_TITLE.PAL, a picture of the IFF kind whose colours are 8 bits). The choices are "Run Introduction", "Create Character", "Continue Game" and "About Martian Dreams".
 *
 * APPROX(89): the introduction is a slide show: the backdrops (the 320 x 128 pictures) of SCENE1, SCENE2 and SCENE3.LZC and the lines of SCENETXT.LZC items 1-3, the lines spread evenly over the
 * backdrops of their scene; the original moves the figures (the other items of the scene files) and times the lines by the code of MARTIAN.EXE, which is not traced. The creation of a character
 * (CREATE.LZC) only asks for a name and the credits only say so; the keys, the place of the frame (the text lines are at y 133, 150, 167 and 184, the frame 3 above them) and the message are not traced.
 */
class MdFrontEnd {
public:
	enum State { MENU, NAME, INTRO, DONE, QUIT };
	enum { CHOICES = 4 };
	/** The pages of the introduction that were loaded (0 before it is first asked for). */
	size_t introPages() const {
		return _pages.size();
	}
	size_t introPage() const {
		return _page;
	}

	explicit MdFrontEnd(const std::string &dir);

	bool ok() const {
		return _ok;
	}
	State state() const {
		return _state;
	}
	void render(Framebuffer &screen);
	/** Feeds a key; false when the menu is finished (see wantsGame()). */
	bool handleKey(int key);
	bool wantsGame() const {
		return _state == DONE;
	}
	/** The name typed for a new character ("" when the saved game is continued). */
	const std::string &name() const {
		return _name;
	}
	int selected() const {
		return _selected;
	}

private:
	bool nested(const Bytes &item, size_t k, Shape &out) const;
	bool loadIntro();
	void wrap(const std::string &text, std::vector<std::string> &lines) const;

	struct Page {
		size_t picture; // index into _pictures
		std::string text;
	};
	std::vector<Shape> _pictures; // the backdrops (320 x 128) of the three scenes
	std::vector<Page> _pages;
	size_t _page = 0;
	Palette _gamePalette; // MDPAL, the colours of the scenes

	std::string _dir;
	bool _ok = false;
	State _state = MENU;
	int _selected = 2;
	std::string _message, _name;
	Shape _picture, _frame, _frameOff;
	uint8_t _palette[256][3];
	Font _font;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

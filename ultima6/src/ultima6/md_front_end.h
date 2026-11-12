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
 * APPROX(89): the introduction (SCENE1-4A/B.LZC, drawn by the scripts of SCENETXT.LZC), the creation of a character (CREATE.LZC) and the pages of the credits are not done: the introduction and
 * the credits say so, the creation only asks for a name; the keys, the place of the frame (the text lines are at y 133, 150, 167 and 184, the frame 3 above them) and the message are not traced.
 */
class MdFrontEnd {
public:
	enum State { MENU, NAME, DONE, QUIT };
	enum { CHOICES = 4 };

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

#ifndef ULTIMA6_ULTIMA6_ENDING_H
#define ULTIMA6_ULTIMA6_ENDING_H

#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "files/shape.h"
#include "files/tile_data.h"

#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * What END.EXE shows after the Codex has gone: the Codex chamber (END.SHP 7) with the red gate (3), Lord British and Nystul and King Draxinusom (8, 9), the
 * Codex on the wall (2) and the story on a parchment strip (10) page after page, then the congratulations with the time played in words.
 * The text, the pictures and the palette (ENDPAL.LBM, an IFF palette) are the original's; which picture goes with which page and where they are placed are guesses
 * (the original's scene scripting, `show_ending_text` 10BFA, is not traced). See docs/u6_ending.md and approximations.md.
 */
class Ending {
public:
	/** years, months, days played (the game date less 4 / 7 / 161, borrowing 28 days and 13 months). */
	Ending(const std::string &dir, int years, int months, int days);

	bool ok() const {
		return _ok;
	}
	void render(Framebuffer &screen);
	/** A key; false when the ending is over. */
	bool handleKey(int key);

	/** The time played as the credits say it: "in two years, five months,\nand three days." pieces (number_to_words, 12D1C). */
	static std::string numberToWords(int n);
	static std::string timePlayed(int years, int months, int days);

private:
	bool _ok = false;
	struct Screen {
		int scene; // the page of ending_text.inc it comes from
		std::vector<std::string> lines;
	};
	std::vector<Screen> _screens;
	size_t _page = 0;
	int _years, _months, _days;
	Font _font;
	std::vector<Shape> _shapes;
	uint8_t _palette[256][3];
};

} // namespace Ultima6
} // namespace Ultima6

#endif

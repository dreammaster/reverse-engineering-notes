// Martian Dreams' menu (MDMENU.LZC, MD_TITLE.PAL): the four choices, the frame that moves, the name of a new character. Skipped without the game data.

#include "ultima6/md_front_end.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::MdFrontEnd;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

int main() {
	MdFrontEnd menu("md/game/MARTIAN");
	if (!menu.ok()) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	CHECK(menu.state() == MdFrontEnd::MENU && menu.selected() == 2);
	menu.handleKey(KEY_DOWN);
	CHECK(menu.selected() == 3);
	menu.handleKey(KEY_DOWN);
	CHECK(menu.selected() == 0);
	menu.handleKey(KEY_UP);
	CHECK(menu.selected() == 3);
	Framebuffer screen;
	menu.render(screen);
	int lit = 0;
	for (int i = 0; i < Framebuffer::WIDTH * Framebuffer::HEIGHT; ++i)
		lit += screen.pixels()[i] != 0;
	CHECK(lit > 5000); // the picture is there
	// the introduction: a page for each line of the three scenes, any key goes on, the end is the menu again; the credits only say a line; Create Character asks for a name
	CHECK(menu.handleKey('1') && menu.state() == MdFrontEnd::INTRO && menu.introPages() > 30);
	menu.render(screen);
	lit = 0;
	for (int i = 0; i < Framebuffer::WIDTH * 128; ++i)
		lit += screen.pixels()[i] != 0;
	CHECK(lit > 4000); // a backdrop
	size_t pages = menu.introPages();
	for (size_t i = 0; i + 1 < pages; ++i)
		menu.handleKey(' ');
	CHECK(menu.state() == MdFrontEnd::INTRO && menu.introPage() == pages - 1);
	menu.handleKey(' ');
	CHECK(menu.state() == MdFrontEnd::MENU);
	CHECK(menu.handleKey('1') && menu.state() == MdFrontEnd::INTRO && menu.handleKey(KEY_ESC) && menu.state() == MdFrontEnd::MENU);
	CHECK(menu.handleKey('4') && menu.state() == MdFrontEnd::MENU);
	CHECK(menu.handleKey('2') && menu.state() == MdFrontEnd::NAME);
	for (char c : std::string("Tesla"))
		menu.handleKey(c);
	menu.handleKey(KEY_BACKSPACE);
	menu.handleKey('a');
	CHECK(!menu.handleKey(KEY_ENTER) && menu.wantsGame() && menu.name() == "Tesla");
	MdFrontEnd second("md/game/MARTIAN");
	CHECK(!second.handleKey('3') && second.wantsGame() && second.name().empty());
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

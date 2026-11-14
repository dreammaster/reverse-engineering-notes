// Martian Dreams' menu (MDMENU.LZC, MD_TITLE.PAL): the four choices, the frame that moves, the name of a new character. Skipped without the game data.

#include "files/world_data.h"
#include "ultima6/md_front_end.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::MdFrontEnd;
using ::Ultima6::Ultima6::MdIntro;
using ::Ultima6::Ultima6::MdCreation;
using ::Ultima6::Ultima6::MdNewCharacter;

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
	// the introduction is the script of MARTIAN.EXE's routine: it fades in on the study, shows lines, and ends by itself (a key ends it too)
	CHECK(menu.handleKey('1') && menu.state() == MdFrontEnd::INTRO && menu.animating());
	menu.tick(2000);
	menu.render(screen);
	lit = 0;
	for (int i = 0; i < Framebuffer::WIDTH * 128; ++i)
		lit += screen.pixels()[i] != 0;
	CHECK(lit > 4000); // a backdrop
	CHECK(menu.intro() && !menu.intro()->lines().empty() && menu.intro()->lines()[0] == "Come in.");
	menu.tick(4000);
	CHECK(menu.intro()->lines().size() >= 1 && menu.intro()->lines()[0] != "Come in.");
	int guard = 0;
	while (menu.animating() && guard++ < 2000)
		menu.tick(500);
	CHECK(menu.state() == MdFrontEnd::MENU && guard < 2000 && guard > 100); // some ten minutes of pictures
	CHECK(menu.handleKey('1') && menu.state() == MdFrontEnd::INTRO && menu.handleKey(KEY_ESC) && menu.state() == MdFrontEnd::MENU);
	// the credits are a script of their own; the ending too (MdIntro::ENDING), both end by themselves
	CHECK(menu.handleKey('4') && menu.state() == MdFrontEnd::INTRO && menu.animating());
	guard = 0;
	while (menu.animating() && guard++ < 2000)
		menu.tick(500);
	CHECK(menu.state() == MdFrontEnd::MENU && guard < 2000 && guard > 40);
	CHECK(menu.handleKey('4') && menu.handleKey(KEY_ESC) && menu.state() == MdFrontEnd::MENU);
	{
		MdIntro ending("md/game/MARTIAN", MdIntro::ENDING);
		CHECK(ending.ok() && !ending.finished());
		ending.tick(7000);
		CHECK(!ending.lines().empty());
		ending.runToEnd();
		CHECK(ending.finished());
	}
	// Dr. Freud's questions: A to every question, the name Tesla (a letter too many, taken away again); the scenes go on by themselves and wait for the keys
	CHECK(menu.handleKey('2') && menu.state() == MdFrontEnd::CREATE && menu.animating());
	int steps = 0;
	bool typedName = false;
	while (menu.state() == MdFrontEnd::CREATE && steps++ < 20000) {
		menu.tick(100);
		const MdCreation *c = menu.creation();
		if (c && c->typing()) {
			if (!typedName) {
				typedName = true;
				for (char k : std::string("Teslab"))
					menu.handleKey(k);
				menu.handleKey(KEY_BACKSPACE);
				menu.handleKey(KEY_ENTER);
			}
		} else {
			menu.handleKey('a');
		}
	}
	for (int i = 0; i < 300 && menu.state() != MdFrontEnd::MENU; ++i)
		menu.tick(100);
	CHECK(typedName && menu.state() == MdFrontEnd::MENU && menu.hasNewCharacter());
	const MdNewCharacter &ch = menu.character();
	CHECK(ch.name == "Tesla" && !ch.female);
	CHECK(ch.strength >= 18 && ch.strength <= 26 && ch.dexterity >= 18 && ch.dexterity <= 26 && ch.intelligence >= 18 && ch.intelligence <= 26);
	// a second character is refused with the original's message
	CHECK(menu.handleKey('2') && menu.state() == MdFrontEnd::MENU);
	Framebuffer asked;
	menu.render(asked);
	CHECK(!menu.handleKey('3') && menu.wantsGame());
	MdFrontEnd second("md/game/MARTIAN");
	CHECK(!second.handleKey('3') && second.wantsGame() && !second.hasNewCharacter());
	{ // the character is put into the saved game: actor 1's stats, level 4, 600 experience, the sex in the body's type
		ObjList list;
		Bytes d;
		CHECK(readFile("md/game/MARTIAN/SAVEGAME/OBJLIST", d));
		d.erase(d.begin() + 0x14F1, d.begin() + 0x15F1);
		CHECK(list.load(d));
		MdNewCharacter w = ch;
		w.female = true;
		w.applyTo(list);
		CHECK(list.strength[1] == w.strength && list.level[1] == 4 && list.exp[1] == 600 && list.hp[1] == (uint8_t)(96 + 2 * w.strength));
		CHECK((list.objFrame[1] & 0x3FF) == 0x158 && std::string((const char *)list.partyNames[0]) == "Tesla");
	}
	// the questions are a decision tree: B to everything gives a woman or a man by the first answer, and reaches the last scene
	{
		MdCreation b("md/game/MARTIAN", 5);
		CHECK(b.ok());
		int guard = 0;
		bool typedB = false;
		while (!b.finished() && !b.cancelled() && guard++ < 20000) {
			b.tick(100);
			if (b.typing()) {
				if (!typedB) {
					typedB = true;
					for (char k : std::string("freud"))
						b.key(k);
					b.key(KEY_ENTER);
				}
			} else {
				b.key('b');
			}
		}
		CHECK(b.finished() && b.result().female && b.result().name == "freud");
	}
	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

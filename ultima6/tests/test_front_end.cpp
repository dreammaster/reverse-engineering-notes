// The main menu and character creation, driven by keys (skipped when the game data is absent).

#include "ultima6/front_end.h"
#include "ultima6/session.h"

#include <cstdio>
#include <string>

using namespace Ultima6;
using ::Ultima6::Ultima6::FrontEnd;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static void type(FrontEnd &f, const std::string &s) {
	for (char c : s)
		f.handleKey(c);
}

int main() {
	FrontEnd f("u6/game");
	if (!f.ok()) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	Framebuffer screen;
	f.render(screen);

	// the introduction: Introduction is the first menu entry; its eleven pages draw and it returns to the menu
	{
		FrontEnd intro("u6/game");
		intro.handleKey(KEY_UP);
		intro.handleKey(KEY_ENTER);
		int pages = 0;
		while (pages < 40 && intro.state() == FrontEnd::INTRO) {
			intro.render(screen);
			intro.handleKey(KEY_SPACE);
			++pages;
		}
		CHECK(pages == 11);
		CHECK(intro.state() == FrontEnd::MENU);
	}

	// Journey Onward without a character plays the saved game that ships with the game; the menu starts on "Create a Character".
	f.handleKey(KEY_ENTER);
	type(f, "Gwenno");
	f.handleKey(KEY_ENTER);
	f.handleKey('f');
	f.handleKey(KEY_RIGHT);
	f.handleKey(KEY_ENTER); // portrait
	for (int i = 0; i < 7; ++i) {
		f.render(screen); // every question page draws
		f.handleKey(i % 2 ? 'a' : 'b');
	}
	CHECK(f.hasNewCharacter());
	const ::Ultima6::Ultima6::NewCharacter &c = f.character();
	CHECK(c.name == "Gwenno");
	CHECK(c.female);
	CHECK(c.portrait == 1);
	int total = c.stats.strength + c.stats.dexterity + c.stats.intelligence;
	CHECK(total >= 45 && total <= 66);
	f.render(screen);
	f.handleKey(KEY_ENTER);  // back to the menu, on Journey Onward
	CHECK(!f.handleKey(KEY_ENTER));
	CHECK(f.wantsGame());

	World w;
	CHECK(w.load("u6/game", &f.character()));
	Game g(w);
	CHECK(std::string((const char *)w.actors.partyNames[0]) == "Gwenno");

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

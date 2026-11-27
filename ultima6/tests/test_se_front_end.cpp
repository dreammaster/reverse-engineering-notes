// Savage Empire's front end (SAVAGE.EXE): the menu, the Introduction, the Acknowledgements, Intanya's questions, the ending, the dark moonstone that ends the game. Skipped without the game data.

#include "files/world_data.h"
#include "ultima6/se_front_end.h"
#include "ultima6/session.h"

#include <cstdio>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::SeFrontEnd;
using ::Ultima6::Ultima6::SeNewCharacter;
using ::Ultima6::Ultima6::World;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static std::string joined(const SeFrontEnd &f) {
	std::string s;
	for (const std::string &l : f.textLines())
		s += l + "|";
	return s;
}

// the text with its lines run together (the window breaks lines where it likes)
static bool says(const SeFrontEnd &f, const std::string &text) {
	std::string s;
	for (const std::string &l : f.textLines())
		s += l + " ";
	return s.find(text) != std::string::npos;
}

static void settle(SeFrontEnd &f, int ms) {
	for (int i = 0; i < ms; i += 10)
		f.tick(10);
}

int main() {
	SeFrontEnd menu("se/game/SAVAGE", 1);
	if (!menu.ok()) {
		std::printf("skip (no game data)\n");
		return 0;
	}
	// the menu: three choices until a character is made, the arrows wrap, the picture is there
	CHECK(menu.state() == SeFrontEnd::MENU && menu.selected() == 0 && menu.choices() == 3);
	menu.handleKey(KEY_UP);
	CHECK(menu.selected() == 2);
	menu.handleKey(KEY_DOWN);
	CHECK(menu.selected() == 0);
	Framebuffer screen;
	menu.render(screen);
	int lit = 0;
	for (int i = 0; i < Framebuffer::WIDTH * Framebuffer::HEIGHT; ++i)
		lit += screen.pixels()[i] != 0;
	CHECK(lit > 30000);
	// 'j' (Journey Onward) is not offered without a character; with one it ends the menu
	menu.handleKey('j');
	CHECK(menu.state() == SeFrontEnd::MENU);
	menu.setSaveState(true, false);
	CHECK(menu.choices() == 4);
	CHECK(!menu.handleKey('j') && menu.wantsGame());

	// the Introduction: ten pages, the first one is the dream; Escape leaves it for the menu with the second choice
	{
		SeFrontEnd f("se/game/SAVAGE", 1);
		CHECK(f.handleKey('s') && f.state() == SeFrontEnd::SCENE && f.scene() == SeFrontEnd::INTRODUCTION);
		settle(f, 2500);
		CHECK(f.waitingForKey());
		CHECK(joined(f).find("disturbing dreams") != std::string::npos);
		f.handleKey(' ');
		CHECK(says(f, "Lord British commanded"));
		for (int i = 0; i < 8; ++i)
			f.handleKey(' ');
		CHECK(says(f, "struggling to regain"));
		f.handleKey(' ');
		settle(f, 5000);
		CHECK(f.state() == SeFrontEnd::MENU && f.selected() == 1);
		f.handleKey('s');
		settle(f, 2500);
		f.handleKey(KEY_ESC);
		settle(f, 5000);
		CHECK(f.state() == SeFrontEnd::MENU && f.selected() == 1);
	}

	// the Acknowledgements scroll by themselves and a key does not stop them (a key ends only the animations)
	{
		SeFrontEnd f("se/game/SAVAGE", 1);
		CHECK(f.handleKey('a') && f.scene() == SeFrontEnd::ACKNOWLEDGEMENTS);
		int guard = 0;
		while (f.state() == SeFrontEnd::SCENE && guard++ < 2000)
			settle(f, 100);
		CHECK(f.state() == SeFrontEnd::MENU && f.selected() == 2 && guard > 40);
	}

	// Intanya's questions: the name Tesla, A to everything; the stats are 16 to 18 and three to five more for each answer that raises them
	{
		SeFrontEnd f("se/game/SAVAGE", 3);
		CHECK(f.handleKey('c') && f.scene() == SeFrontEnd::CREATION);
		settle(f, 3000);
		CHECK(f.waitingForKey() && says(f, "dirt-floored hut"));
		f.handleKey(' ');
		CHECK(says(f, "Yes, warrior"));
		f.handleKey(' ');
		f.handleKey(' ');
		CHECK(f.typing() && says(f, "What does the warrior"));
		for (char c : std::string("Teslab?"))
			f.handleKey(c); // the question mark is not a letter of a name
		f.handleKey(KEY_BACKSPACE);
		f.handleKey(KEY_ENTER);
		CHECK(!f.typing());
		int answered = 0;
		for (int step = 0; step < 4000 && f.state() == SeFrontEnd::SCENE; ++step) {
			if (f.waitingForKey()) {
				f.handleKey('a');
				++answered;
			}
			settle(f, 100);
		}
		CHECK(answered >= 3 && f.state() == SeFrontEnd::MENU && f.hasNewCharacter() && f.choices() == 4);
		const SeNewCharacter &c = f.character();
		if (c.name != "Tesla")
			std::printf("name [%s]\n", c.name.c_str());
		CHECK(c.name == "Tesla");
		CHECK(c.strength >= 16 && c.strength <= 33 && c.dexterity >= 16 && c.dexterity <= 33 && c.intelligence >= 16 && c.intelligence <= 33);
		CHECK(c.strength + c.dexterity + c.intelligence >= 48 + 9);
		// the character goes into the objlist as sub_15A7C does
		ObjList list;
		Bytes d;
		CHECK(readFile("se/game/SAVAGE/SAVEGAME/OBJLIST", d) && list.load(d));
		c.applyTo(list);
		CHECK(list.globals[4] == 1 && list.globals[5] == 1 && list.globals[6] == 0 && list.globals[0x23] == 8);
		CHECK(list.strength[1] == c.strength && list.hp[1] == (uint8_t)(2 * c.strength + c.intelligence) && list.globals[0x81] == 1);
		CHECK(std::string((const char *)list.partyNames[0]) == "Tesla" && (list.talkFlags[2] & 1));
		// a game in progress refuses a second character
		SeFrontEnd g("se/game/SAVAGE", 3);
		g.setSaveState(true, true);
		g.handleKey('c');
		settle(g, 3000);
		CHECK(says(g, "already in progress"));
	}

	// the ending: pages of ENDGAME.LZC, the time played in words, Aiela's pages only for 'A'
	{
		SeFrontEnd f("se/game/SAVAGE", 1);
		f.startEnding(5, 3, 1, 'A');
		settle(f, 3000);
		CHECK(f.waitingForKey() && joined(f).find("Members of all the tribes") != std::string::npos);
		bool aiela = false;
		for (int i = 0; i < 40 && f.state() == SeFrontEnd::SCENE; ++i) {
			if (joined(f).find("You say farewell to Aiela") != std::string::npos)
				aiela = true;
			if (joined(f).find("CONGRATULATIONS") != std::string::npos) {
				CHECK(joined(f).find("3 months,|5 days and one year.|") != std::string::npos);
				CHECK(joined(f).find("Communicate your success") != std::string::npos);
			}
			f.handleKey(' ');
			settle(f, 3000);
		}
		CHECK(aiela && f.state() == SeFrontEnd::DONE);
		SeFrontEnd n("se/game/SAVAGE", 1);
		n.startEnding(0, 1, 0, 0);
		bool tristia = false, congrats = false;
		for (int i = 0; i < 40 && n.state() == SeFrontEnd::SCENE; ++i) {
			settle(n, 3000);
			tristia = tristia || joined(n).find("Tristia") != std::string::npos;
			if (joined(n).find("CONGRATULATIONS") != std::string::npos) {
				congrats = true;
				CHECK(joined(n).find("one month.|") != std::string::npos);
			}
			n.handleKey(' ');
		}
		CHECK(!tristia && congrats);
	}

	// the dark moonstone (0x93): destroyed, it ends the game with the time played
	{
		static World w;
		w.game = GameId::SavageEmpire;
		if (w.load("se/game/SAVAGE")) {
			w.objects.reserve(w.objects.size() + 64);
			Game g(w);
			World::Obj o;
			o.status = 0;
			o.x = 0x60;
			o.y = 0x60;
			o.z = 1;
			o.type = 0x93;
			o.frame = 0;
			o.quantity = 20;
			o.quality = 0;
			o.actor = -1;
			o.parent = -1;
			w.objects.push_back(o);
			w.addToMap((int)w.objects.size() - 1);
			int idx = (int)w.objects.size() - 1;
			g.seHurtThing(idx, 5);
			CHECK(!g.endingStarted() && w.objects[idx].quantity == 15);
			g.seHurtThing(idx, 30);
			CHECK(g.endingStarted());
			int d, b, y;
			char who;
			g.seElapsed(d, b, y, who);
			CHECK(d >= 0 && b >= 0 && y >= 0 && who == 0);
			w.actors.talkFlags[0x33] |= 0x20;
			g.seElapsed(d, b, y, who);
			CHECK(who == 'T');
			w.actors.talkFlags[2] |= 0x20;
			g.seElapsed(d, b, y, who);
			CHECK(who == 'A');
		}
	}
	if (failures)
		std::printf("%d failures\n", failures);
	else
		std::printf("ok\n");
	return failures ? 1 : 0;
}

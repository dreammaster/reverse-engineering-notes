// xanth_play <game folder> <out.ppm> "<script>": plays a new game headless with scripted clicks and writes the final screen (with the mouse pointer) as a PPM.
// Script: actions separated by ';' - "c X Y" (click at X, Y on the 320x200 screen), "m X Y" (move the pointer), "a N" (let N animation frames of 100 ms pass),
// "t CONV PIC STEPS" (play conversation CONV over picture PIC pressing Enter, and write the screen when STEPS waits have been answered).
// The same click script, as a HOST_MOUSE timeline, drives the oracle (see tools/compare_play.py).
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>

#include "common/files.h"
#include "xanth/display.h"
#include "xanth/interface.h"

struct Stepper : Xanth::InputSource {
	int budget;
	std::string out;
	Xanth::Display *display;
	Stepper() : budget(-1), display(NULL) {}
	virtual Xanth::InputEvent wait() {
		if (budget == 0) {
			Legend::Surface shown;
			display->render(shown);
			shown.savePpm(out);
			exit(0);
		}
		budget--;
		Xanth::InputEvent e;
		e.type = Xanth::InputEvent::Key;
		e.key = 0x0D;
		return e;
	}
};

int main(int argc, char **argv) {
	if (argc < 4) {
		printf("usage: xanth_play <game folder> <out.ppm> \"c X Y;m X Y;a N\"\n");
		return 2;
	}
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!gf.setFolder(argv[1]) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("cannot read the game files in %s\n", argv[1]);
		return 1;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str)) {
		printf("cannot load OBJECT.DAT / XANTHSTR.DAT\n");
		return 1;
	}
	Xanth::Display display(gf);
	if (!display.init()) {
		printf("cannot load the interface pictures and fonts\n");
		return 1;
	}
	Xanth::Game game(display, messages, names);
	display.attach(&game);
	game.newGame();
	Xanth::Interface ui(game, display);
	Stepper stepper;
	ui.start();
	std::stringstream script(argv[3]);
	std::string action;
	while (std::getline(script, action, ';')) {
		std::stringstream a(action);
		std::string op;
		int x = 0, y = 0;
		a >> op >> x >> y;
		if (op == "t") {
			int steps = 0;
			a >> steps;
			stepper.budget = steps;
			stepper.out = argv[2];
			stepper.display = &display;
			display.setInput(&stepper);
			game.show_compendium(x, y);
			display.setInput(NULL);
		} else if (op == "c")
			ui.click(x, y);
		else if (op == "m")
			ui.move(x, y);
		else if (op == "a")
			for (int i = 0; i < x; i++)
				display.advance(100);
	}
	Legend::Surface shown;
	display.render(shown);
	return shown.savePpm(argv[2]) ? 0 : 1;
}

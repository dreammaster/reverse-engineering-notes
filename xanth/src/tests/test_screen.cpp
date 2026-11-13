// The standalone game on screen: a new game, entering the bedroom, rendered by Display and compared with an oracle screenshot of the same moment
// (XANTH_ORACLE_SHOT=<ppm from engine/host/play.sh>; XANTH_SHOTS=<folder> receives our screen and a diff).
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "tests/screen_compare.h"
#include "xanth/display.h"

int main() {
	const char *dir = getenv("XANTH_DATA");
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!dir || !gf.setFolder(dir) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("test_screen: XANTH_DATA not set, skipped\n");
		return 0;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str)) {
		printf("test_screen: cannot load OBJECT.DAT / XANTHSTR.DAT\n");
		return 1;
	}
	Xanth::Display display(gf);
	if (!display.init()) {
		printf("test_screen: cannot load the interface pictures and fonts\n");
		return 1;
	}
	Xanth::Game game(display, messages, names);
	display.attach(&game);
	game.newGame();
	static const char *kVerbs[] = {"Take", "Put", "Look at", "Open", "Close", "Talk to", "Look"};
	display.setVerbs(std::vector<std::string>(kVerbs, kVerbs + 7));
	display.redrawAll();
	game.go_to_room(game.state.v.current_room);
	display.advance(0);          // the oracle's screenshot was taken two frames into the rain
	display.advance(100);

	int failures = 0;
	int roomPixels = 0;                       // the picture window must show the room, not the marble
	for (int y = 4; y < 126; y++)
		for (int x = 51; x < 315; x++)
			roomPixels += display.screen().get(x, y) >= 80;
	if (roomPixels < 5000) {
		printf("FAIL: the picture window shows no room (%d pixels)\n", roomPixels);
		failures++;
	}
	const char *shots = getenv("XANTH_SHOTS");
	if (shots)
		display.screen().savePpm(std::string(shots) + "/game.ppm");
	const char *oracleShot = getenv("XANTH_ORACLE_SHOT");
	std::vector<byte> oracle;
	if (oracleShot && loadPpm(oracleShot, oracle)) {
		printf("new game, bedroom, against %s:\n", oracleShot);
		std::string diffPath = shots ? std::string(shots) + "/game_diff.ppm" : std::string();
		int diff = compareScreens(display.screen(), oracle, shots ? diffPath.c_str() : NULL);
		// the oracle's screen also has the mouse cursor (51 pixels) and a hover label (about 400) that the game logic does not draw
		if (diff > 500) {
			printf("FAIL: %d pixels differ\n", diff);
			failures++;
		}
	}
	printf("test_screen: %d failures\n", failures);
	return failures ? 1 : 0;
}

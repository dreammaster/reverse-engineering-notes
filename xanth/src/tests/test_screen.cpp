// The standalone game on screen: new games driven by clicks, rendered by Display and compared with oracle screenshots of the same moments
// (engine/host/play.sh "<mouse script>" <steps> out.ppm). Environment: XANTH_DATA (game files), XANTH_SHOTS (folder for our screens and diffs) and the oracle's
// screens XANTH_ORACLE_SHOT (the opening), XANTH_ORACLE_LOOK (click Look at, then the computer), XANTH_ORACLE_WALK (click the north-west arrow of the compass).
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "common/files.h"
#include "tests/screen_compare.h"
#include "xanth/display.h"
#include "xanth/interface.h"

static int g_failures = 0;

struct World {
	Xanth::Display display;
	std::unique_ptr<Xanth::Game> game;
	std::unique_ptr<Xanth::Interface> ui;
	World(const Common::GameFiles &gf) : display(gf) {}
};

static std::unique_ptr<World> newWorld(const Common::GameFiles &gf, const Legend::MessageFile &messages, const Xanth::ObjectNames &names) {
	std::unique_ptr<World> w(new World(gf));
	if (!w->display.init())
		return std::unique_ptr<World>();
	w->game.reset(new Xanth::Game(w->display, messages, names));
	w->display.attach(w->game.get());
	w->game->newGame();
	w->ui.reset(new Xanth::Interface(*w->game, w->display));
	w->ui->start();
	return w;
}

/** Compares with an oracle screenshot (if its path is in the environment); `allowed` is the number of differing pixels that are known (animation phase, cursor, hover label). */
static void check(World &w, const char *what, const char *envName, const char *file, int allowed) {
	const char *shots = getenv("XANTH_SHOTS");
	if (shots)
		w.display.screen().savePpm(std::string(shots) + "/" + file + ".ppm");
	const char *path = getenv(envName);
	std::vector<byte> oracle;
	if (!path || !loadPpm(path, oracle))
		return;
	printf("%s, against %s:\n", what, path);
	std::string diffPath = shots ? std::string(shots) + "/" + file + "_diff.ppm" : std::string();
	int diff = compareScreens(w.display.screen(), oracle, shots ? diffPath.c_str() : NULL);
	if (diff > allowed) {
		printf("FAIL: %s: %d pixels differ (at most %d expected)\n", what, diff, allowed);
		g_failures++;
	}
}

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
	if (getenv("XANTH_MSG")) {
		int b = 0, i = 0;
		sscanf(getenv("XANTH_MSG"), "%d,%d", &b, &i);
		std::string m = messages.string(MSG(b, i));
		for (size_t k = 0; k < m.size(); k++)
			printf(m[k] >= 32 && m[k] < 127 ? "%c" : "<%02X>", (unsigned char)m[k]);
		printf("\n");
	}

	{   // the opening: a new game in the bedroom
		std::unique_ptr<World> w = newWorld(gf, messages, names);
		if (!w) {
			printf("test_screen: cannot load the interface pictures and fonts\n");
			return 1;
		}
		w->display.advance(0);          // the oracle's screenshot was taken two frames into the rain
		w->display.advance(100);
		int roomPixels = 0;             // the picture window must show the room, not the marble
		for (int y = 4; y < 126; y++)
			for (int x = 51; x < 315; x++)
				roomPixels += w->display.screen().get(x, y) >= 80;
		if (roomPixels < 5000) {
			printf("FAIL: the picture window shows no room (%d pixels)\n", roomPixels);
			g_failures++;
		}
		// the oracle's screen also has the mouse cursor (51 pixels) and a hover label (about 400) that the game logic does not draw
		check(*w, "the bedroom", "XANTH_ORACLE_SHOT", "game", 500);

		// click "Look at", then the computer: the answer is in the grown text window (the rain, cursor and hover label differ: about 1,200 pixels)
		w->ui->click(20, 28);
		w->ui->click(160, 100);
		check(*w, "look at the computer", "XANTH_ORACLE_LOOK", "game_look", 1300);
	}
	{   // walk: the north-west arrow of the compass leads to the entrance
		std::unique_ptr<World> w = newWorld(gf, messages, names);
		w->ui->click(11, 137);
		check(*w, "walk north-west", "XANTH_ORACLE_WALK", "game_walk", 1300);
	}
	printf("test_screen: %d failures\n", g_failures);
	return g_failures ? 1 : 0;
}

// Random keys at the running game of each of the three games (skipped when the data is absent): the game must go on without crashing, however the commands are mixed,
// and every thousand keys the screen is drawn. The seeds are fixed so a failure repeats.

#include "ultima6/session.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace Ultima6;
using ::Ultima6::Ultima6::Game;
using ::Ultima6::Ultima6::World;

static const int KEYS[] = {KEY_UP,   KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_ENTER, KEY_ESC, KEY_SPACE, KEY_BACKSPACE, KEY_NONE};

static int pick(unsigned &state) {
	state = state * 1664525u + 1013904223u;
	unsigned r = state >> 8;
	switch (r % 5) {
	case 0:
		return KEYS[(r >> 4) % (sizeof(KEYS) / sizeof(KEYS[0]))];
	case 1:
		return 'a' + (int)((r >> 4) % 26);
	case 2:
		return '0' + (int)((r >> 4) % 10);
	case 3:
		return KEYS[(r >> 4) % 4]; // mostly walking
	default:
		return "uglatmdrcsbnyxwekphjvzfiq"[(r >> 4) % 25];
	}
}

static int fuzz(const char *dir, GameId id, unsigned seed, int steps) {
	static World w;
	w = World();
	w.game = id;
	if (!w.load(dir))
		return -1;
	std::unique_ptr<Game> g(new Game(w));
	static Framebuffer screen;
	unsigned state = seed;
	int restarts = 0;
	for (int i = 0; i < steps; ++i) {
		int key = pick(state);
		if (key == 'q' || key == 'Q') // the quit command asks for confirmation; skip the letter itself
			key = KEY_ESC;
		if (!g->handleKey(key)) {
			++restarts;
			w = World();
			w.game = id;
			if (!w.load(dir))
				return -1;
			g.reset(new Game(w));
		}
		if (i % 1000 == 0)
			g->render(screen);
	}
	return restarts;
}

int main(int argc, char **argv) {
	int steps = argc > 1 ? std::atoi(argv[1]) : 1500, seeds = argc > 2 ? std::atoi(argv[2]) : 2;
	struct Run {
		const char *dir;
		GameId id;
	};
	const Run runs[] = {{"u6/game", GameId::Ultima6}, {"md/game/MARTIAN", GameId::MartianDreams}, {"se/game/SAVAGE", GameId::SavageEmpire}};
	int failures = 0;
	for (const Run &r : runs) {
		for (unsigned seed = 1; seed <= (unsigned)seeds; ++seed) {
			int restarts = fuzz(r.dir, r.id, seed * 7919u, steps);
			if (restarts < 0) {
				std::printf("skip %s (no game data)\n", r.dir);
				break;
			}
			std::printf("%s seed %u: %d restarts\n", r.dir, seed, restarts);
		}
	}
	std::printf(failures ? "FAILED\n" : "ok\n");
	return failures ? 1 : 0;
}

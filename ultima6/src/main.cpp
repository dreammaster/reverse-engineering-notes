#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "engine/game_id.h"
#include "ultima6/front_end.h"
#include "ultima6/session.h"
#include "ultima6/world.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

static void usage(const char *prog) {
	std::fprintf(stderr, "usage: %s <game directory> [--play] [--save dir] [--keys <script>] [--shot file.png] [--scale n]\n", prog);
	std::fprintf(stderr, "  --play   skip the main menu and start from the saved game\n");
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	Ultima6::GameId id = Ultima6::detectGame(argv[1]);
	if (id == Ultima6::GameId::Unknown) {
		std::fprintf(stderr, "%s: no Ultima VI / Martian Dreams / Savage Empire found\n", argv[1]);
		return 1;
	}
	std::printf("%s\n", Ultima6::gameInfo(id).title);
	if (id != Ultima6::GameId::Ultima6)
		return 0; // the spin-offs are not playable yet

	const char *keys = nullptr, *shot = nullptr;
	int scale = 3;
	bool play = false;
	std::string saveDir = "u6save"; // our own save directory; the game's files are never written
	for (int i = 2; i < argc; ++i) {
		if (!std::strcmp(argv[i], "--keys") && i + 1 < argc)
			keys = argv[++i];
		else if (!std::strcmp(argv[i], "--shot") && i + 1 < argc)
			shot = argv[++i];
		else if (!std::strcmp(argv[i], "--save") && i + 1 < argc)
			saveDir = argv[++i];
		else if (!std::strcmp(argv[i], "--play"))
			play = true;
		else if (!std::strcmp(argv[i], "--scale") && i + 1 < argc)
			scale = std::atoi(argv[++i]);
		else {
			usage(argv[0]);
			return 2;
		}
	}

	std::unique_ptr<Ultima6::Backend> backend;
	if (keys || shot)
		backend.reset(new Ultima6::HeadlessBackend(keys ? keys : ""));
	else
		backend.reset(Ultima6::createWindowBackend("Ultima VI", scale));
	if (!backend) {
		std::fprintf(stderr, "no window available; use --keys/--shot\n");
		return 1;
	}
	Ultima6::Framebuffer screen;

	// The menu and character creation (U.EXE), then the game (GAME.EXE).
	Ultima6::Ultima6::World world;
	bool created = false;
	Ultima6::Ultima6::NewCharacter character;
	if (!play) {
		Ultima6::Ultima6::FrontEnd front(argv[1]);
		if (!front.ok()) {
			std::fprintf(stderr, "cannot load the menu files\n");
			return 1;
		}
		bool running = true;
		while (running) {
			front.render(screen);
			backend->present(screen);
			int key = backend->waitKey();
			running = front.handleKey(key);
		}
		if (!front.wantsGame()) {
			if (shot)
				screen.savePng(shot, scale);
			return 0;
		}
		created = front.hasNewCharacter();
		if (created)
			character = front.character();
	}

	Ultima6::Bytes probe;
	bool haveSave = Ultima6::readFile(Ultima6::joinPath(saveDir, "OBJLIST"), probe);
	if (!world.load(argv[1], created ? &character : nullptr, !created && haveSave ? saveDir : std::string())) {
		std::fprintf(stderr, "cannot load %s\n", world.error().c_str());
		return 1;
	}
	Ultima6::Ultima6::Game game(world);
	game.saveDir = saveDir;

	bool running = true;
	while (running) {
		game.render(screen);
		backend->present(screen);
		int key = backend->waitKey();
		running = game.handleKey(key);
	}

	if (shot) {
		game.render(screen);
		if (!screen.savePng(shot, scale)) {
			std::fprintf(stderr, "cannot write %s\n", shot);
			return 1;
		}
		std::printf("wrote %s, party at %d,%d,%d, %02d:%02d day %d month %d year %d\n", shot, game.x, game.y, game.z, game.clock.hour,
					game.clock.minute, game.clock.day, game.clock.month, game.clock.year);
	}
	return 0;
}

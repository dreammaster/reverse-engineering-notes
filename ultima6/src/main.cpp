#include "engine/backend.h"
#include "engine/framebuffer.h"
#include "engine/game_id.h"
#include "engine/sfx.h"
#include "files/music.h"
#include "ultima6/ending.h"
#include "ultima6/front_end.h"
#include "ultima6/md_front_end.h"
#include "ultima6/md_create.h"
#include "ultima6/md_intro.h"
#include "ultima6/session.h"
#include "ultima6/world.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

// Starts and stops the AdLib music as the game asks (the track numbers are those of the list at ds:2955 of GAME.EXE); U6_NO_MUSIC in the environment keeps it silent.
class MusicSwitch {
public:
	MusicSwitch(Ultima6::Backend &backend, const std::string &dir) : _backend(backend), _dir(dir), _playing(0) {
	}
	void set(int track) {
		static const char *const NAMES[12] = {"ULTIMA", "BOOTUP", "INTRO", "CREATE", "FOREST", "HORNPIPE", "ENGAGE", "STONES", "DUNGEON", "BRIT", "GARGOYLE", "END"};
		if (track == _playing)
			return;
		_playing = track;
		_backend.stopMusic();
		if (track < 1 || track > 12 || std::getenv("U6_NO_MUSIC"))
			return;
		Ultima6::Bytes file;
		Ultima6::MusicTrack t;
		if (Ultima6::readFile(Ultima6::joinPath(_dir, std::string(NAMES[track - 1]) + ".M"), file) && t.load(file))
			_backend.playMusic(t.data);
	}

private:
	Ultima6::Backend &_backend;
	std::string _dir;
	int _playing;
};

static void usage(const char *prog) {
	std::fprintf(stderr, "usage: %s <game directory> [--play] [--ending] [--save dir] [--keys <script>] [--shot file.png] [--scale n]\n", prog);
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
	// Martian Dreams and Savage Empire: only the world (map, objects, tiles) is shared with Ultima VI so far; there are no menus, no frame, no text and no conversations yet,
	// the saved game is walked about in

	const char *keys = nullptr, *shot = nullptr;
	int scale = 3;
	bool play = id == Ultima6::GameId::SavageEmpire, endingOnly = false; // Martian Dreams has its menu; Savage Empire's saved game is walked about in
	std::string saveDir = id == Ultima6::GameId::MartianDreams ? "mdsave" : id == Ultima6::GameId::SavageEmpire ? "sesave" : "u6save"; // our own save directory; the game's files are never written
	for (int i = 2; i < argc; ++i) {
		if (!std::strcmp(argv[i], "--keys") && i + 1 < argc)
			keys = argv[++i];
		else if (!std::strcmp(argv[i], "--shot") && i + 1 < argc)
			shot = argv[++i];
		else if (!std::strcmp(argv[i], "--save") && i + 1 < argc)
			saveDir = argv[++i];
		else if (!std::strcmp(argv[i], "--play"))
			play = true;
		else if (!std::strcmp(argv[i], "--ending"))
			endingOnly = true;
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
	MusicSwitch music(*backend, argv[1]);

	// --ending: the ending alone, for a game of one year, two months and three days
	if (endingOnly && id == Ultima6::GameId::MartianDreams) { // Martian Dreams' ending: the script of MARTIAN.EXE's routine sub_15C84
		Ultima6::Ultima6::MdIntro ending(argv[1], Ultima6::Ultima6::MdIntro::ENDING);
		if (!ending.ok()) {
			std::fprintf(stderr, "cannot load the ending files\n");
			return 1;
		}
		unsigned last = backend->ticks();
		while (!ending.finished()) {
			ending.render(screen);
			backend->present(screen);
			if (backend->pollKey() != Ultima6::KEY_NONE)
				break;
			unsigned now = backend->ticks();
			ending.tick(now - last);
			last = now;
			backend->delay(15);
		}
		if (shot)
			screen.savePng(shot, scale);
		return 0;
	}
	if (endingOnly) {
		Ultima6::Ultima6::Ending ending(argv[1], 1, 2, 3);
		if (!ending.ok()) {
			std::fprintf(stderr, "cannot load the ending files\n");
			return 1;
		}
		bool running = true;
		music.set(12);
		while (running) {
			ending.render(screen);
			backend->present(screen);
			running = ending.handleKey(backend->waitKey());
		}
		if (shot)
			screen.savePng(shot, scale);
		return 0;
	}

	// The menu and character creation (U.EXE), then the game (GAME.EXE).
	Ultima6::Ultima6::World world;
	world.game = id;
	bool created = false;
	Ultima6::Ultima6::NewCharacter character;
	Ultima6::Ultima6::MdNewCharacter mdCharacter; // Martian Dreams: the character made in its menu
	bool mdCreated = false;
	if (!play && id == Ultima6::GameId::MartianDreams) {
		Ultima6::Ultima6::MdFrontEnd front(argv[1]);
		if (!front.ok()) {
			std::fprintf(stderr, "cannot load the menu files\n");
			return 1;
		}
		bool running = true;
		while (running) {
			front.render(screen);
			backend->present(screen);
			int key;
			if (front.animating()) { // the introduction goes on by itself, a key ends it
				unsigned last = backend->ticks();
				while ((key = backend->pollKey()) == Ultima6::KEY_NONE) {
					unsigned now = backend->ticks();
					front.tick(now - last);
					last = now;
					if (!front.animating())
						break;
					front.render(screen);
					backend->present(screen);
					backend->delay(15);
				}
				if (key == Ultima6::KEY_NONE)
					continue;
			} else {
				key = backend->waitKey();
			}
			running = front.handleKey(key);
		}
		if (!front.wantsGame()) {
			if (shot)
				screen.savePng(shot, scale);
			return 0;
		}
		mdCreated = front.hasNewCharacter();
			mdCharacter = front.character();
	} else if (!play) {
		Ultima6::Ultima6::FrontEnd front(argv[1]);
		if (!front.ok()) {
			std::fprintf(stderr, "cannot load the menu files\n");
			return 1;
		}
		if (const char *env = std::getenv("U6_TRANSFER_DIR"))
			front.transferDir = env;
		bool running = true;
		while (running) {
			music.set(front.music());
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
	if (mdCreated) // the character of Martian Dreams' questions is put into the saved game as the original's creation does
		mdCharacter.applyTo(world.actors);
	Ultima6::Ultima6::Game game(world);
	game.saveDir = saveDir;
	Ultima6::BorlandRandom sfxRandom(7);
	game.sound = [&](int effect, int param) {
		if (!std::getenv("U6_NO_MUSIC"))
			backend->playSound(Ultima6::Sfx::render(effect, param, sfxRandom));
	};

	bool running = true;
	const bool idleAnimation = !(keys || shot); // a window: the map goes on moving (water, fire, the cycling palette) while a key is waited for
	unsigned lastTick = backend->ticks();
	while (running) {
		music.set(game.music());
		game.render(screen);
		backend->present(screen);
		int key;
		if (idleAnimation) {
			while ((key = backend->pollKey()) == Ultima6::KEY_NONE) {
				unsigned now = backend->ticks();
				if (now - lastTick >= 125) { // about eight redraws a second (APPROX 75)
					lastTick = now;
					game.animate();
					music.set(game.music());
					game.render(screen);
					backend->present(screen);
				} else {
					backend->delay(10);
				}
			}
		} else {
			key = backend->waitKey();
		}
		if (key == Ultima6::KEY_MOUSE_LEFT || key == Ultima6::KEY_MOUSE_RIGHT)
			key = game.mouseKey(backend->mouseX, backend->mouseY, key == Ultima6::KEY_MOUSE_RIGHT);
		running = game.handleKey(key);
	}

	if (game.endingStarted()) {
		int years, months, days;
		game.timePlayed(years, months, days);
		Ultima6::Ultima6::Ending ending(argv[1], years, months, days);
		if (ending.ok()) {
			bool showing = true;
			music.set(12);
			while (showing) {
				ending.render(screen);
				backend->present(screen);
				showing = ending.handleKey(backend->waitKey());
			}
		}
	}

	music.set(0);
	if (shot) {
		game.render(screen);
		if (!screen.savePng(shot, scale)) {
			std::fprintf(stderr, "cannot write %s\n", shot);
			return 1;
		}
		std::printf("wrote %s, party at %d,%d,%d, %02d:%02d day %d month %d year %d\n", shot, game.x, game.y, game.z, game.gameClock.hour,
					game.gameClock.minute, game.gameClock.day, game.gameClock.month, game.gameClock.year);
	}
	return 0;
}

#include "ultima/ultima6/engine/std_headers.h"

#include "ultima/ultima6/ultima6_engine.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "common/config-manager.h"
#include "common/events.h"
#include "common/fs.h"
#include "common/mutex.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/paletteman.h"

#include "ultima/ultima6/engine/backend.h"
#include "ultima/ultima6/engine/framebuffer.h"
#include "ultima/ultima6/engine/game_id.h"
#include "ultima/ultima6/engine/music_player.h"
#include "ultima/ultima6/engine/random.h"
#include "ultima/ultima6/engine/sfx.h"
#include "ultima/ultima6/files/data_file.h"
#include "ultima/ultima6/files/music.h"
#include "ultima/ultima6/ultima6/ending.h"
#include "ultima/ultima6/ultima6/front_end.h"
#include "ultima/ultima6/ultima6/session.h"
#include "ultima/ultima6/ultima6/world.h"

namespace Ultima {
namespace Ultima6 {

// The AdLib driver (MusicPlayer) and the sound effects, as a stream the mixer pulls from its own thread.
class SoundStream : public Audio::AudioStream {
public:
	SoundStream() : _effectPos(0) {
	}

	void play(const Bytes &track) {
		Common::StackLock lock(_mutex);
		_player.stop();
		_player.play(track);
	}

	void stop() {
		Common::StackLock lock(_mutex);
		_player.stop();
	}

	void effect(const std::vector<int16_t> &samples) {
		Common::StackLock lock(_mutex);
		_effect = samples;
		_effectPos = 0;
	}

	int readBuffer(int16 *buffer, const int numSamples) override {
		Common::StackLock lock(_mutex);
		_player.render(buffer, numSamples);
		for (int n = 0; n < numSamples && _effectPos < _effect.size(); ++n, ++_effectPos) {
			int v = buffer[n] + _effect[_effectPos];
			buffer[n] = (int16)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
		}
		return numSamples;
	}

	bool isStereo() const override {
		return false;
	}
	int getRate() const override {
		return Opl2::SAMPLE_RATE;
	}
	bool endOfData() const override {
		return false;
	}

private:
	Common::Mutex _mutex;
	MusicPlayer _player;
	std::vector<int16_t> _effect;
	size_t _effectPos;
};

// The screen, the input and the sound of ScummVM for the games.
class ScummBackend : public Backend {
public:
	ScummBackend() : _stream(nullptr), _quit(false) {
	}

	~ScummBackend() override {
		if (_stream) {
			g_system->getMixer()->stopHandle(_handle);
			delete _stream;
		}
	}

	void present(const Framebuffer &screen) override {
		byte palette[256 * 3];
		for (int i = 0; i < 256; ++i) {
			int r, g, b;
			screen.colour(i, r, g, b);
			palette[i * 3] = (byte)r;
			palette[i * 3 + 1] = (byte)g;
			palette[i * 3 + 2] = (byte)b;
		}
		g_system->getPaletteManager()->setPalette(palette, 0, 256);
		g_system->copyRectToScreen(screen.pixels(), Framebuffer::WIDTH, 0, 0, Framebuffer::WIDTH, Framebuffer::HEIGHT);
		g_system->updateScreen();
		pump();
	}

	int pollKey() override {
		pump();
		if (_quit)
			return KEY_QUIT;
		if (_keys.empty())
			return KEY_NONE;
		Input in = _keys.front();
		_keys.erase(_keys.begin());
		mouseX = in.x;
		mouseY = in.y;
		return in.key;
	}

	int waitKey() override {
		for (;;) {
			int k = pollKey();
			if (k != KEY_NONE)
				return k;
			g_system->delayMillis(10);
		}
	}

	unsigned ticks() override {
		return g_system->getMillis();
	}

	void delay(unsigned ms) override {
		g_system->delayMillis(ms);
		pump();
	}

	void playMusic(const std::vector<uint8_t> &track) override {
		open();
		_stream->play(track);
	}

	void stopMusic() override {
		if (_stream)
			_stream->stop();
	}

	void playSound(const std::vector<int16_t> &samples) override {
		open();
		_stream->effect(samples);
	}

private:
	struct Input {
		int key, x, y;
	};

	void open() {
		if (_stream)
			return;
		_stream = new SoundStream;
		g_system->getMixer()->playStream(Audio::Mixer::kMusicSoundType, &_handle, _stream, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
	}

	void push(int key, int x = 0, int y = 0) {
		Input in = {key, x, y};
		_keys.push_back(in);
	}

	// The same keys the window of the stand-alone program gives: the characters, and the cursor keys.
	void pump() {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			switch (e.type) {
			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				_quit = true;
				break;
			case Common::EVENT_LBUTTONDOWN:
				push(KEY_MOUSE_LEFT, e.mouse.x, e.mouse.y);
				break;
			case Common::EVENT_RBUTTONDOWN:
				push(KEY_MOUSE_RIGHT, e.mouse.x, e.mouse.y);
				break;
			case Common::EVENT_KEYDOWN:
				key(e.kbd);
				break;
			default:
				break;
			}
		}
	}

	void key(const Common::KeyState &k) {
		switch (k.keycode) {
		case Common::KEYCODE_UP: push(KEY_UP); return;
		case Common::KEYCODE_DOWN: push(KEY_DOWN); return;
		case Common::KEYCODE_LEFT: push(KEY_LEFT); return;
		case Common::KEYCODE_RIGHT: push(KEY_RIGHT); return;
		case Common::KEYCODE_HOME: push(KEY_HOME); return;
		case Common::KEYCODE_END: push(KEY_END); return;
		case Common::KEYCODE_PAGEUP: push(KEY_PGUP); return;
		case Common::KEYCODE_PAGEDOWN: push(KEY_PGDN); return;
		case Common::KEYCODE_ESCAPE: push(KEY_ESC); return;
		case Common::KEYCODE_RETURN:
		case Common::KEYCODE_KP_ENTER: push(KEY_ENTER); return;
		case Common::KEYCODE_BACKSPACE: push(KEY_BACKSPACE); return;
		default: break;
		}
		if (k.ascii == 32 || (k.ascii >= 33 && k.ascii < 127))
			push(k.ascii);
	}

	SoundStream *_stream;
	Audio::SoundHandle _handle;
	bool _quit;
	std::vector<Input> _keys;
};

// Starts and stops the AdLib music as the game asks (the track numbers are those of the list at ds:2955 of GAME.EXE).
class MusicSwitch {
public:
	explicit MusicSwitch(Backend &backend) : _backend(backend), _playing(0) {
	}

	void set(int track) {
		static const char *const NAMES[12] = {"ULTIMA", "BOOTUP", "INTRO", "CREATE", "FOREST", "HORNPIPE", "ENGAGE", "STONES", "DUNGEON", "BRIT", "GARGOYLE", "END"};
		if (track == _playing)
			return;
		_playing = track;
		_backend.stopMusic();
		if (track < 1 || track > 12)
			return;
		Bytes file;
		MusicTrack t;
		if (readFile(std::string(NAMES[track - 1]) + ".M", file) && t.load(file))
			_backend.playMusic(t.data);
	}

private:
	Backend &_backend;
	int _playing;
};

static const char *SAVE_DIR = "@save/"; // see files/data_file.cpp: the saved games are kept by ScummVM

Ultima6Engine::Ultima6Engine(OSystem *syst, const UltimaGameDescription *gameDesc) : Engine(syst), _gameDescription(gameDesc) {
}

Ultima6Engine::~Ultima6Engine() {
}

bool Ultima6Engine::hasFeature(EngineFeature f) const {
	return f == kSupportsReturnToLauncher;
}

Common::Error Ultima6Engine::run() {
	const Common::FSNode gameDataDir(ConfMan.getPath("path"));
	SearchMan.addDirectory(gameDataDir, 0, 4);

	initGraphics(Framebuffer::WIDTH, Framebuffer::HEIGHT);

	// an arrow, as the window of the stand-alone program has
	static const byte ARROW[12 * 8] = {
		1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0,
		1, 2, 1, 0, 0, 0, 0, 0,
		1, 2, 2, 1, 0, 0, 0, 0,
		1, 2, 2, 2, 1, 0, 0, 0,
		1, 2, 2, 2, 2, 1, 0, 0,
		1, 2, 2, 2, 2, 2, 1, 0,
		1, 2, 2, 2, 1, 1, 1, 1,
		1, 2, 1, 2, 1, 0, 0, 0,
		1, 1, 0, 1, 2, 1, 0, 0,
		1, 0, 0, 1, 2, 1, 0, 0,
		0, 0, 0, 0, 1, 1, 0, 0};
	static const byte CURSOR_PALETTE[3 * 3] = {0, 0, 0, 0, 0, 0, 255, 255, 255};
	CursorMan.pushCursor(ARROW, 8, 12, 0, 0, 0);
	CursorMan.pushCursorPalette(CURSOR_PALETTE, 0, 3);
	CursorMan.showMouse(true);

	GameId id;
	switch (_gameDescription->gameId) {
	case GAME_MARTIAN_DREAMS:
		id = GameId::MartianDreams;
		break;
	case GAME_SAVAGE_EMPIRE:
		id = GameId::SavageEmpire;
		break;
	default:
		id = GameId::Ultima6;
		break;
	}

	ScummBackend backend;
	Framebuffer screen;
	MusicSwitch music(backend);

	// The menu and character creation (U.EXE), then the game (GAME.EXE).
	// Martian Dreams and Savage Empire have no menus of their own yet: the saved game is walked about in.
	Ultima6::World world;
	world.game = id;
	bool created = false;
	Ultima6::NewCharacter character;
	if (id == GameId::Ultima6) {
		Ultima6::FrontEnd front("");
		if (!front.ok())
			return Common::Error(Common::kReadingFailed, "cannot load the menu files");
		bool running = true;
		while (running && !shouldQuit()) {
			music.set(front.music());
			front.render(screen);
			backend.present(screen);
			int key = backend.waitKey();
			if (key == KEY_QUIT)
				return Common::kNoError;
			running = front.handleKey(key);
		}
		if (!front.wantsGame())
			return Common::kNoError;
		created = front.hasNewCharacter();
		if (created)
			character = front.character();
	}

	Bytes probe;
	bool haveSave = readFile(joinPath(SAVE_DIR, "OBJLIST"), probe);
	if (!world.load("", created ? &character : nullptr, !created && haveSave ? std::string(SAVE_DIR) : std::string()))
		return Common::Error(Common::kReadingFailed, world.error().c_str());
	Ultima6::Game game(world);
	game.saveDir = SAVE_DIR;
	BorlandRandom sfxRandom(7);
	game.sound = [&](int effect, int param) {
		backend.playSound(Sfx::render(effect, param, sfxRandom));
	};

	bool running = true;
	unsigned lastTick = backend.ticks();
	while (running && !shouldQuit()) {
		music.set(game.music());
		game.render(screen);
		backend.present(screen);
		int key;
		while ((key = backend.pollKey()) == KEY_NONE) {
			unsigned now = backend.ticks();
			if (now - lastTick >= 125) { // about eight redraws a second (APPROX 75)
				lastTick = now;
				game.animate();
				music.set(game.music());
				game.render(screen);
				backend.present(screen);
			} else {
				backend.delay(10);
			}
		}
		if (key == KEY_QUIT)
			break;
		if (key == KEY_MOUSE_LEFT || key == KEY_MOUSE_RIGHT)
			key = game.mouseKey(backend.mouseX, backend.mouseY, key == KEY_MOUSE_RIGHT);
		running = game.handleKey(key);
	}

	if (game.endingStarted() && !shouldQuit()) {
		int years, months, days;
		game.timePlayed(years, months, days);
		Ultima6::Ending ending("", years, months, days);
		if (ending.ok()) {
			bool showing = true;
			music.set(12);
			while (showing) {
				ending.render(screen);
				backend.present(screen);
				int key = backend.waitKey();
				if (key == KEY_QUIT)
					break;
				showing = ending.handleKey(key);
			}
		}
	}

	music.set(0);
	return Common::kNoError;
}

} // namespace Ultima6
} // namespace Ultima

#ifndef ULTIMA6_ENGINE_BACKEND_H
#define ULTIMA6_ENGINE_BACKEND_H

#include "engine/framebuffer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Ultima6 {

/** Keys the game sees: printable characters are their ASCII code. */
enum Key {
	KEY_NONE = 0,
	KEY_ESC = 27,
	KEY_ENTER = 13,
	KEY_BACKSPACE = 8,
	KEY_SPACE = 32,
	KEY_UP = 256,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_HOME,
	KEY_END,
	KEY_PGUP,
	KEY_PGDN,
	KEY_MOUSE_LEFT,  // a click: Backend::mouseX / mouseY say where (in the 320 x 200 screen)
	KEY_MOUSE_RIGHT,
	KEY_QUIT // window closed
};

/** Screen, keyboard and clock of the machine the game runs on. */
class Backend {
public:
	virtual ~Backend() {}

	/** Shows the frame. */
	virtual void present(const Framebuffer &screen) = 0;
	/** Next key, or KEY_NONE when none is waiting. */
	virtual int pollKey() = 0;
	/** Waits for a key (or KEY_QUIT). */
	virtual int waitKey() = 0;
	/** Milliseconds. */
	virtual unsigned ticks() = 0;
	virtual void delay(unsigned ms) = 0;

	/** Music: the AdLib track (MusicTrack::data) starts from its beginning; the one playing is stopped first. */
	virtual void playMusic(const std::vector<uint8_t> &) {}
	/** snd_stop. */
	virtual void stopMusic() {}

	int mouseX = 0, mouseY = 0; // where the last click was
};

/** Scripted keys, no window: for tests and screenshots. Keys are described by Backend::parseKeys. */
class HeadlessBackend : public Backend {
public:
	explicit HeadlessBackend(const std::string &script);

	void present(const Framebuffer &screen) override {
		_last = screen;
	}
	int pollKey() override;
	int waitKey() override;
	unsigned ticks() override {
		return _clock;
	}
	void delay(unsigned ms) override {
		_clock += ms;
	}
	const Framebuffer &lastFrame() const {
		return _last;
	}
	bool exhausted() const {
		return _pos >= _keys.size();
	}

	/**
	 * Script syntax: plain characters, and {up} {down} {left} {right} {enter} {esc} {space} {bs}.
	 */
	static std::string parseKeysDoc();

private:
	std::vector<int> _keys;
	size_t _pos;
	unsigned _clock;
	Framebuffer _last;
};

/** A window (Windows only; nullptr elsewhere). `scale` multiplies the 320 x 200 screen. */
Backend *createWindowBackend(const char *title, int scale);

} // namespace Ultima6

#endif

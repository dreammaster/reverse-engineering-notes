#pragma once
// Input events for the Wasteland engine, shaped like ScummVM's Common::Event / EventManager (and SDL's event queue): the
// front end (Win32 window, SDL, ScummVM's OSystem) posts Events; Input::pump() dispatches them to the state the original code
// polls -- the BIOS key buffer (int 16h), the mouse driver state (int 33h) and the quit flag. Nothing here depends on an OS API.
#include <cstdint>
#include <deque>

namespace wl {

struct Machine;

// Key codes follow SDL 1.2 / ScummVM's KeyCode numbering for the keys WL.EXE reads; printable keys use their lower case ASCII.
enum KeyCode : int {
	KEY_NONE = 0, KEY_BACKSPACE = 8, KEY_TAB = 9, KEY_RETURN = 13, KEY_ESCAPE = 27, KEY_SPACE = 32,
	KEY_DELETE = 127, KEY_UP = 273, KEY_DOWN = 274, KEY_RIGHT = 275, KEY_LEFT = 276, KEY_INSERT = 277, KEY_HOME = 278, KEY_END = 279,
	KEY_PAGEUP = 280, KEY_PAGEDOWN = 281, KEY_F1 = 282, KEY_F12 = 293
};
enum KeyMod : int { KMOD_SHIFT = 1, KMOD_CTRL = 2, KMOD_ALT = 4 };
enum MouseButton : int { BUTTON_LEFT = 1, BUTTON_RIGHT = 2, BUTTON_MIDDLE = 4 };

struct KeyState {
	int keycode = KEY_NONE;   // KeyCode (or the lower case ASCII of a printable key)
	int ascii = 0;            // the character the key produces (0 for arrows, function keys, ...)
	int mods = 0;             // KeyMod bits
};

struct Event {
	enum Type { None, KeyDown, KeyUp, MouseMove, MouseDown, MouseUp, Tick, Quit };
	Type type = None;
	KeyState key;          // KeyDown / KeyUp
	int x = 0, y = 0;      // mouse position in game pixels (0..319, 0..199) for the Mouse* events
	int button = 0;        // MouseButton for MouseDown / MouseUp
};

// Source of events; a front end can implement this and let Input::pollSource() drain it (EventManager::pollEvent).
class EventSource {
public:
	virtual ~EventSource() = default;
	virtual bool pollEvent(Event &event) = 0;
};

class Input {
public:
	bool mouseEnabled = false;   // the mouse driver is "installed" (int 33h answers); off for tests that compare against the oracle
	int mouseX = 160, mouseY = 100;
	uint8_t buttons = 0;         // MouseButton bits currently held
	bool quit = false;           // a Quit event was dispatched
	uint64_t ticks = 0;          // Tick events dispatched
	int rangeMaxX = 0x130, rangeMaxY = 0xC1;   // int 33h AX=7/8

	void post(const Event &event) { queue.push_back(event); }
	size_t pending() const { return queue.size(); }
	void pollSource(EventSource &source) { Event e; while (source.pollEvent(e)) post(e); }
	// Dispatches every queued event: keys into Machine::keys (BIOS format), mouse into the driver state
	void pump(Machine &m);
	// The BIOS key word (scan code << 8 | ascii) of a key press; 0 when the key has no BIOS equivalent
	static uint16_t biosKey(const KeyState &key);

private:
	std::deque<Event> queue;
};

} // namespace wl

#include "wasteland1/input.h"
#include "wasteland1/machine.h"
#include <algorithm>
#include <cstring>

namespace wl {

namespace {
uint8_t asciiScan(int c) {
	static const char *rows[] = {"1234567890-=", "qwertyuiop[]", "asdfghjkl;'`", "zxcvbnm,./"};
	static const uint8_t first[] = {2, 16, 30, 44};
	if (c >= 'A' && c <= 'Z') c += 32;
	for (int r = 0; r < 4; ++r)
		if (const char *p = std::strchr(rows[r], c); p && c) return uint8_t(first[r] + (p - rows[r]));
	switch (c) {
	case ' ': return 0x39;
	case 13: return 0x1C;
	case 27: return 0x01;
	case 8: return 0x0E;
	case 9: return 0x0F;
	case 92: return 0x2B;   // backslash
	default: return 0;
	}
}
}

uint16_t Input::biosKey(const KeyState &k) {
	if (k.keycode >= KEY_F1 && k.keycode <= KEY_F12) {
		const int n = k.keycode - KEY_F1;
		uint8_t scan = n < 10 ? uint8_t(0x3B + n) : uint8_t(0x85 + (n - 10));
		if (n < 10) {   // BIOS scan codes of the shifted function keys
			if (k.mods & KMOD_ALT) scan = uint8_t(0x68 + n);
			else if (k.mods & KMOD_CTRL) scan = uint8_t(0x5E + n);
			else if (k.mods & KMOD_SHIFT) scan = uint8_t(0x54 + n);
		}
		return uint16_t(scan << 8);
	}
	switch (k.keycode) {
	case KEY_UP: return 0x4800;
	case KEY_DOWN: return 0x5000;
	case KEY_LEFT: return 0x4B00;
	case KEY_RIGHT: return 0x4D00;
	case KEY_HOME: return 0x4700;
	case KEY_END: return 0x4F00;
	case KEY_PAGEUP: return 0x4900;
	case KEY_PAGEDOWN: return 0x5100;
	case KEY_INSERT: return 0x5200;
	case KEY_DELETE: return 0x5300;
	default: break;
	}
	int ascii = k.ascii;
	if (ascii == 0 && k.keycode >= KEY_BACKSPACE && k.keycode < 127) ascii = k.keycode;
	if (ascii <= 0 || ascii >= 128) return 0;
	return uint16_t((asciiScan(ascii) << 8) | ascii);
}

void Input::pump(Machine &m) {
	while (!queue.empty()) {
		const Event e = queue.front();
		queue.pop_front();
		switch (e.type) {
		case Event::KeyDown:
			if (const uint16_t word = biosKey(e.key)) m.keys.push_back(word);
			break;
		case Event::MouseMove:
			mouseX = e.x;
			mouseY = e.y;
			break;
		case Event::MouseDown:
			mouseX = e.x;
			mouseY = e.y;
			buttons = uint8_t(buttons | e.button);
			break;
		case Event::MouseUp:
			mouseX = e.x;
			mouseY = e.y;
			buttons = uint8_t(buttons & ~e.button);
			break;
		case Event::Tick: ++ticks; break;
		case Event::Quit: quit = true; break;
		default: break;
		}
	}
}

} // namespace wl

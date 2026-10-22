/* The SDL2 backend: the only code in the engine that touches a platform API
 * (together with the tools that use it). It presents the video driver's displayed
 * page, turns SDL events into mouse events for the driver and a DOS-style key queue,
 * and turns the driver's busy-wait loops into timed waits. A ScummVM port replaces
 * this file with an OSystem-based equivalent. */

#ifndef YENDOR1_PLATFORM_SDLHOST_H
#define YENDOR1_PLATFORM_SDLHOST_H

#include "vid/Video.h"

#include <deque>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Yendor1 {
namespace Platform {

class SdlHost : public Vid::Host {
public:
	SdlHost();
	~SdlHost();

	/** Open a window showing `video`'s displayed page. `scale` multiplies 640 x 480 (the 4:3 screen shape). */
	bool open(Vid::Video &video, int scale, const char *title);
	void close();

	/** Process pending events (mouse, keys, window). Returns false once the window was closed. */
	bool pump();
	/** Redraw the window from the video driver's displayed page. */
	void present();

	/** DOS INT 21h/AH=06h style: next byte of the key queue, or -1. Extended keys arrive as 0 then the scan code. */
	int pollKey();
	void flushKeys() { _keys.clear(); }
	bool quitRequested() const { return _quit; }

	// Vid::Host
	void delay(Vid::uint16 loopCount);
	void pageShown(int page);

private:
	void pushKey(int ascii, int scan);

	Vid::Video *_video;
	SDL_Window *_window;
	SDL_Renderer *_renderer;
	SDL_Texture *_texture;
	std::vector<Shared::byte> _rgb;
	std::deque<int> _keys;
	bool _quit;
	unsigned _lastPresent;
	double _delayCarryMs;
	int _winW, _winH;
};

} // namespace Platform
} // namespace Yendor1

#endif

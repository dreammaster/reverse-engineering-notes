#include "platform/SdlHost.h"

#include <SDL.h>

namespace Yendor1 {
namespace Platform {

SdlHost::SdlHost()
	: _video(0), _window(0), _renderer(0), _texture(0), _quit(false), _lastPresent(0), _delayCarryMs(0), _winW(0), _winH(0) {
}

SdlHost::~SdlHost() {
	close();
}

bool SdlHost::open(Vid::Video &video, int scale, const char *title) {
	_video = &video;
	SDL_SetMainReady();                  // tools build with SDL_MAIN_HANDLED (no SDL2main)
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0 && SDL_Init(SDL_INIT_VIDEO) != 0)
		return false;
	_winW = 640 * scale;
	_winH = 480 * scale;
	_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _winW, _winH, SDL_WINDOW_RESIZABLE);
	if (!_window)
		return false;
	_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!_renderer)
		_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_SOFTWARE);
	if (!_renderer)
		return false;
	SDL_RenderSetLogicalSize(_renderer, 640, 480);     // the EGA mode is shown on a 4:3 screen
	_texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, Vid::kScreenWidth, Vid::kScreenHeight);
	if (!_texture)
		return false;
	_rgb.resize((size_t)Vid::kScreenWidth * Vid::kScreenHeight * 3);
	SDL_ShowCursor(SDL_DISABLE);                         // the game draws its own cursor
	_video->setHost(this);
	present();
	return true;
}

void SdlHost::close() {
	if (_video)
		_video->setHost(0);
	if (_texture)
		SDL_DestroyTexture(_texture);
	if (_renderer)
		SDL_DestroyRenderer(_renderer);
	if (_window)
		SDL_DestroyWindow(_window);
	_texture = 0;
	_renderer = 0;
	_window = 0;
	_video = 0;
}

void SdlHost::present() {
	if (!_video || !_texture)
		return;
	_video->renderRgb(_video->displayedPage(), &_rgb[0]);
	SDL_UpdateTexture(_texture, 0, &_rgb[0], Vid::kScreenWidth * 3);
	SDL_RenderClear(_renderer);
	SDL_Rect dst = {0, 0, 640, 480};
	SDL_RenderCopy(_renderer, _texture, 0, &dst);
	SDL_RenderPresent(_renderer);
	_lastPresent = SDL_GetTicks();
}

void SdlHost::pageShown(int) {
	present();
}

void SdlHost::delay(Vid::uint16 loopCount) {
	// A `loop` instruction took ~17 cycles on the machines SW.EXE calibrates for; call it ~0.4 us.
	_delayCarryMs += loopCount * 0.0004;
	if (_delayCarryMs >= 1.0) {
		const unsigned ms = (unsigned)_delayCarryMs;
		_delayCarryMs -= ms;
		SDL_Delay(ms);
	}
}

void SdlHost::pushKey(int ascii, int scan) {
	if (ascii) {
		_keys.push_back(ascii);
	} else {
		_keys.push_back(0);
		_keys.push_back(scan);
	}
}

int SdlHost::pollKey() {
	if (_keys.empty())
		return -1;
	const int k = _keys.front();
	_keys.pop_front();
	return k;
}

bool SdlHost::pump() {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			_quit = true;
			break;
		case SDL_MOUSEMOTION: {
			// window -> 640x350 EGA coordinates (the 4:3 logical size stretches 350 rows over 480)
			float lx, ly;
			SDL_RenderWindowToLogical(_renderer, e.motion.x, e.motion.y, &lx, &ly);
			int x = (int)lx;
			int y = (int)(ly * Vid::kScreenHeight / 480.0f);
			_video->mouseEvent(true, x - _video->ctx.mouseX, y - _video->ctx.mouseY, 0, 0);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
			_video->mouseEvent(false, 0, 0, e.button.button == SDL_BUTTON_LEFT ? 1 : 0, e.button.button == SDL_BUTTON_RIGHT ? 1 : 0);
			break;
		case SDL_KEYDOWN: {
			const SDL_Keycode k = e.key.keysym.sym;
			if (k == SDLK_RETURN || k == SDLK_KP_ENTER) pushKey(0x0D, 0x1C);
			else if (k == SDLK_ESCAPE) pushKey(0x1B, 0x01);
			else if (k == SDLK_BACKSPACE) pushKey(0x08, 0x0E);
			else if (k == SDLK_TAB) pushKey(0x09, 0x0F);
			else if (k == SDLK_UP) pushKey(0, 0x48);
			else if (k == SDLK_DOWN) pushKey(0, 0x50);
			else if (k == SDLK_LEFT) pushKey(0, 0x4B);
			else if (k == SDLK_RIGHT) pushKey(0, 0x4D);
			else if (k == SDLK_HOME) pushKey(0, 0x47);
			else if (k == SDLK_END) pushKey(0, 0x4F);
			else if (k == SDLK_PAGEUP) pushKey(0, 0x49);
			else if (k == SDLK_PAGEDOWN) pushKey(0, 0x51);
			else if (k >= SDLK_F1 && k <= SDLK_F10) pushKey(0, 0x3B + (k - SDLK_F1));
			break;
		}
		case SDL_TEXTINPUT:
			for (const char *p = e.text.text; *p; p++)
				if ((unsigned char)*p >= 0x20 && (unsigned char)*p < 0x7F)
					pushKey((unsigned char)*p, 0);
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_EXPOSED)
				present();
			break;
		default:
			break;
		}
	}
	if (SDL_GetTicks() - _lastPresent > 33)
		present();
	return !_quit;
}

} // namespace Platform
} // namespace Yendor1

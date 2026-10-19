#ifdef USE_REAL_SDL

#include "graphicslib/sdlGraphics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppGlobals.h"
#include "TSpriteHandle.h"
#include "soundengine/audioMixer.h"

TSdlGraphics::TSdlGraphics(const TGraphicsStartup &startup) : _startup(startup) {
	// DEPONIA_SHOTS=<directory>:<every>:<count>
	if (const char *shots = std::getenv("DEPONIA_SHOTS")) {
		std::string text(shots);
		size_t last = text.rfind(':');
		size_t middle = last == std::string::npos ? std::string::npos : text.rfind(':', last - 1);

		if (middle != std::string::npos) {
			_shotDirectory = text.substr(0, middle);
			_shotEvery = std::atoi(text.substr(middle + 1, last - middle - 1).c_str());
			_shotsLeft = std::atoi(text.substr(last + 1).c_str());
		}
	}

	// DEPONIA_CLICKS=<frame>:<x>:<y>[:<button>];...
	if (const char *clicks = std::getenv("DEPONIA_CLICKS")) {
		std::string text(clicks);
		size_t at = 0;

		while (at < text.size()) {
			size_t end = text.find(';', at);
			std::string item = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
			Click click = {0, 0, 0, 1};

			if (std::sscanf(item.c_str(), "%d:%d:%d:%d", &click.frame, &click.x, &click.y, &click.button) >= 3)
				_clicks.push_back(click);

			if (end == std::string::npos)
				break;

			at = end + 1;
		}
	}
}

TSdlGraphics::~TSdlGraphics() {
	destroyReleased();

	for (auto &entry : _sprites) {
		if (entry.second.texture)
			SDL_DestroyTexture(entry.second.texture);
	}

	if (_renderer)
		SDL_DestroyRenderer(_renderer);
}

// The part of the window that has the proportions of the game's picture, in the middle.
void TSdlGraphics::CalculateDisplayedArea(const wxSize &windowSize, const wxSize &renderSize, wxRect *displayedArea) {
	if (renderSize.width <= 0 || renderSize.height <= 0 || windowSize.width <= 0 || windowSize.height <= 0) {
		displayedArea->x = 0;
		displayedArea->y = 0;
		displayedArea->width = windowSize.width;
		displayedArea->height = windowSize.height;
		return;
	}

	double scale = std::min(static_cast<double>(windowSize.width) / renderSize.width, static_cast<double>(windowSize.height) / renderSize.height);

	displayedArea->width = static_cast<int>(renderSize.width * scale);
	displayedArea->height = static_cast<int>(renderSize.height * scale);
	displayedArea->x = (windowSize.width - displayedArea->width) / 2;
	displayedArea->y = (windowSize.height - displayedArea->height) / 2;
}

bool TSdlGraphics::InitGraphics(const wxSize &windowSize, wxSize &renderSize, wxRect *displayedArea) {
	if (!VSPlayerWindow)
		return false;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, _startup.nearestNeighbor ? "0" : "1");

	_renderer = SDL_CreateRenderer(VSPlayerWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!_renderer)
		_renderer = SDL_CreateRenderer(VSPlayerWindow, -1, SDL_RENDERER_SOFTWARE);

	if (!_renderer)
		return false;

	// premultiplied alpha: the colour already has the alpha in it (dst = src + dst * (1 - src alpha)); the software renderer of SDL has no
	// custom blend modes (it would copy the pictures without their alpha): there the pixels are made straight again and SDL's own blend is used
	SDL_RendererInfo info;

	_premultiplied = !(SDL_GetRendererInfo(_renderer, &info) == 0 && info.name && std::strcmp(info.name, "software") == 0);
	_blend = _premultiplied ? SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
	                                                     SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD)
	                       : SDL_BLENDMODE_BLEND;
	_renderSize = renderSize;
	SetSize(renderSize.width, renderSize.height);
	SetSurfaceSize(windowSize.width, windowSize.height);

	SDL_GetWindowSize(VSPlayerWindow, &_windowWidth, &_windowHeight);
	fitToWindow();
	*displayedArea = _area;

	SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
	SDL_RenderClear(_renderer);
	return true;
}

// The picture of the game goes to the part of the window that keeps its proportions: the scale makes the game's coordinates the pixels there.
void TSdlGraphics::fitToWindow() {
	wxSize window;

	window.width = _windowWidth;
	window.height = _windowHeight;
	CalculateDisplayedArea(window, _renderSize, &_area);

	SDL_Rect viewport;

	viewport.x = _area.x;
	viewport.y = _area.y;
	viewport.w = _area.width;
	viewport.h = _area.height;
	SDL_RenderSetViewport(_renderer, &viewport);
	SDL_RenderSetScale(_renderer, static_cast<float>(_area.width) / static_cast<float>(_renderSize.width),
	                   static_cast<float>(_area.height) / static_cast<float>(_renderSize.height));
	g_displayedArea = _area;
	SetSurfaceSize(_windowWidth, _windowHeight);
}

void TSdlGraphics::ToggleWindowMode() {
	if (!VSPlayerWindow)
		return;

	Uint32 flags = SDL_GetWindowFlags(VSPlayerWindow);

	SDL_SetWindowFullscreen(VSPlayerWindow, (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

bool TSdlGraphics::SetWindowSize(int width, int height) {
	if (!VSPlayerWindow)
		return false;

	SDL_SetWindowSize(VSPlayerWindow, width, height);
	return true;
}

bool TSdlGraphics::IsFullscreen() {
	return VSPlayerWindow && (SDL_GetWindowFlags(VSPlayerWindow) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

// Only the pixels are kept (the preloader's threads make sprites too: the renderer is for the thread of the game).
bool TSdlGraphics::CreateSprite(TSpriteHandle **sprite, const char *data, int width, int height, int bytesPerPixel, int pitch, bool flag) {
	if (!TGraphicsInterface::CreateSprite(sprite, data, width, height, bytesPerPixel, pitch, flag))
		return false;

	Sprite made;

	made.width = width;
	made.height = height;
	made.pixels.assign(static_cast<size_t>(width) * height * 4, 0);

	if (data) {
		for (int y = 0; y < height; y++) {
			const unsigned char *line = reinterpret_cast<const unsigned char *>(data) + static_cast<size_t>(y) * pitch;
			unsigned char *out = &made.pixels[static_cast<size_t>(y) * width * 4];

			for (int x = 0; x < width; x++) {
				if (bytesPerPixel >= 4) {
					std::memcpy(out + x * 4, line + x * bytesPerPixel, 4);
				} else if (bytesPerPixel == 3) {
					std::memcpy(out + x * 4, line + x * 3, 3);
					out[x * 4 + 3] = 255;
				}
			}
		}
	}

	std::lock_guard<std::mutex> lock(_spritesLock);

	_sprites[*sprite] = std::move(made);
	return true;
}

void TSdlGraphics::OnSpriteHandleReleased(TSpriteHandle *handle) {
	{
		std::lock_guard<std::mutex> lock(_spritesLock);
		auto found = _sprites.find(handle);

		if (found != _sprites.end()) {
			if (found->second.texture)
				_released.push_back(found->second.texture);

			_sprites.erase(found);
		}
	}

	TGraphicsInterface::OnSpriteHandleReleased(handle);
}

void TSdlGraphics::destroyReleased() {
	std::vector<SDL_Texture *> textures;

	{
		std::lock_guard<std::mutex> lock(_spritesLock);

		textures.swap(_released);
	}

	for (SDL_Texture *texture : textures)
		SDL_DestroyTexture(texture);
}

SDL_Texture *TSdlGraphics::textureOf(Sprite &sprite) {
	if (sprite.texture)
		return sprite.texture;

	if (sprite.width <= 0 || sprite.height <= 0)
		return nullptr;

	SDL_Texture *texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, sprite.width, sprite.height);

	if (!texture)
		return nullptr;

	if (!_premultiplied) {
		// (straight colours for SDL's own blend: the colour goes back up by the alpha)
		for (size_t i = 0; i + 3 < sprite.pixels.size(); i += 4) {
			unsigned alpha = sprite.pixels[i + 3];

			if (alpha != 0 && alpha != 255) {
				for (int channel = 0; channel < 3; channel++)
					sprite.pixels[i + channel] = static_cast<unsigned char>(std::min(255u, sprite.pixels[i + channel] * 255u / alpha));
			}
		}
	}

	SDL_UpdateTexture(texture, nullptr, sprite.pixels.data(), sprite.width * 4);
	SDL_SetTextureBlendMode(texture, _blend);
	sprite.texture = texture;
	sprite.pixels.clear();
	sprite.pixels.shrink_to_fit();
	return texture;
}

void TSdlGraphics::SetOffset(float x, float y) {
	_offsetX = x;
	_offsetY = y;
}

// A picture with its tint and alpha, turned and scaled about the centre as the other backends do (the scale first, then the turn).
void TSdlGraphics::Draw(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
                        const unsigned int &color, int /*shader*/, float rotation, const wxPoint &rotationCenter, float scaleX, float scaleY,
                        int /*matrixId*/) {
	if (!_renderer || destRect.width <= 0.0f || destRect.height <= 0.0f)
		return;

	SDL_Texture *texture = nullptr;

	{
		std::lock_guard<std::mutex> lock(_spritesLock);
		auto found = _sprites.find(sprite);

		if (found == _sprites.end())
			return;

		texture = textureOf(found->second);
	}

	if (!texture)
		return;

	float dx = destRect.x + _offsetX;
	float dy = destRect.y + _offsetY;
	float centerX = rotationCenter.x < 0 ? dx + destRect.width / 2.0f : static_cast<float>(rotationCenter.x) + _offsetX;
	float centerY = rotationCenter.y < 0 ? dy + destRect.height / 2.0f : static_cast<float>(rotationCenter.y) + _offsetY;
	float scaleXs = scaleX == 0.0f ? 1.0f : scaleX;
	float scaleYs = scaleY == 0.0f ? 1.0f : scaleY;

	SDL_FRect destination;

	destination.x = centerX + (dx - centerX) * scaleXs;
	destination.y = centerY + (dy - centerY) * scaleYs;
	destination.w = destRect.width * scaleXs;
	destination.h = destRect.height * scaleYs;

	SDL_Rect source;

	source.x = sourceRect.x;
	source.y = sourceRect.y;
	source.w = sourceRect.width;
	source.h = sourceRect.height;

	// the tint and the alpha: the picture is premultiplied, so its colour goes down with the alpha too
	float tintAlpha = static_cast<float>((color >> 24) & 0xFF) / 255.0f * alpha;

	tintAlpha = std::max(0.0f, std::min(1.0f, tintAlpha));
	float colorScale = _premultiplied ? tintAlpha : 1.0f;

	SDL_SetTextureColorMod(texture, static_cast<Uint8>((color & 0xFF) * colorScale), static_cast<Uint8>(((color >> 8) & 0xFF) * colorScale),
	                       static_cast<Uint8>(((color >> 16) & 0xFF) * colorScale));
	SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(tintAlpha * 255.0f));

	SDL_FPoint center;

	center.x = centerX - destination.x;
	center.y = centerY - destination.y;
	_dirty = true;
	SDL_RenderCopyExF(_renderer, texture, &source, &destination, static_cast<double>(rotation) * 180.0 / 3.14159265358979323846, &center,
	                  mirrored ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void TSdlGraphics::DrawBox(const wxRect &rect, unsigned int color, float alpha) {
	if (!_renderer)
		return;

	float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f * alpha;

	a = std::max(0.0f, std::min(1.0f, a));
	float scale = _premultiplied ? a : 1.0f;

	SDL_SetRenderDrawBlendMode(_renderer, _blend);
	SDL_SetRenderDrawColor(_renderer, static_cast<Uint8>((color & 0xFF) * scale), static_cast<Uint8>(((color >> 8) & 0xFF) * scale),
	                       static_cast<Uint8>(((color >> 16) & 0xFF) * scale), static_cast<Uint8>(a * 255.0f));

	SDL_FRect box;

	box.x = static_cast<float>(rect.x) + _offsetX;
	box.y = static_cast<float>(rect.y) + _offsetY;
	box.w = static_cast<float>(rect.width);
	box.h = static_cast<float>(rect.height);
	_dirty = true;
	SDL_RenderFillRectF(_renderer, &box);
}

void TSdlGraphics::DrawLine(const wxPoint &from, const wxPoint &to, unsigned int color, float alpha) {
	if (!_renderer)
		return;

	float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f * alpha;

	a = std::max(0.0f, std::min(1.0f, a));
	float scale = _premultiplied ? a : 1.0f;

	SDL_SetRenderDrawBlendMode(_renderer, _blend);
	SDL_SetRenderDrawColor(_renderer, static_cast<Uint8>((color & 0xFF) * scale), static_cast<Uint8>(((color >> 8) & 0xFF) * scale),
	                       static_cast<Uint8>(((color >> 16) & 0xFF) * scale), static_cast<Uint8>(a * 255.0f));
	_dirty = true;
	SDL_RenderDrawLineF(_renderer, static_cast<float>(from.x) + _offsetX, static_cast<float>(from.y) + _offsetY, static_cast<float>(to.x) + _offsetX,
	                    static_cast<float>(to.y) + _offsetY);
}

// The frame is shown (once: a transition shows its frames with AfterDrawScene(true, true), the master control's frame calls Swap() after it).
void TSdlGraphics::AfterDrawScene(bool a, bool b) {
	if (a && b)
		Swap();
}

void TSdlGraphics::Swap() {
	if (!_renderer)
		return;

	if (_dirty) {
		if (_shotsLeft > 0 && _shotEvery > 0 && _shown % _shotEvery == 0) {
			SaveShot(_shotDirectory + "/shot_" + std::to_string(_shown) + ".bmp");
			_shotsLeft--;
		}

		_shown++;
		SDL_RenderPresent(_renderer);

		if (std::getenv("DEPONIA_FPS")) {
			unsigned now = SDL_GetTicks();

			if (_fpsStart == 0)
				_fpsStart = now;

			_fpsFrames++;

			if (now - _fpsStart >= 5000) {
				std::fprintf(stderr, "fps: %.1f (%s renderer), audio frames rendered %lld\n", _fpsFrames * 1000.0 / (now - _fpsStart), _premultiplied ? "hardware" : "software", soundengine::AudioMixer::Get().GetRenderedFrames());
				_fpsStart = now;
				_fpsFrames = 0;
			}
		}

		PushClicks();
		_dirty = false;

		// the next frame starts black (the bars round the game's picture too)
		SDL_RenderSetViewport(_renderer, nullptr);
		SDL_RenderSetScale(_renderer, 1.0f, 1.0f);
		SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
		SDL_RenderClear(_renderer);
		destroyReleased();

		// a window that was resized: the picture is put in the new one
		int width = 0, height = 0;

		SDL_GetWindowSize(VSPlayerWindow, &width, &height);

		if (width != _windowWidth || height != _windowHeight) {
			_windowWidth = width;
			_windowHeight = height;
		}

		fitToWindow();
	}

	_offsetX = 0.0f;
	_offsetY = 0.0f;
	SetFadeDrawn(false);
}

// The mouse of a test: a move, the button down and the button up, at the place of the window that the game's place has.
void TSdlGraphics::PushClicks() {
	for (const Click &click : _clicks) {
		if (click.frame != _shown)
			continue;

		int x = _area.x + click.x * _area.width / std::max(1, _renderSize.width);
		int y = _area.y + click.y * _area.height / std::max(1, _renderSize.height);
		SDL_Event event;

		SDL_zero(event);
		event.type = SDL_MOUSEMOTION;
		event.motion.x = x;
		event.motion.y = y;
		SDL_PushEvent(&event);

		for (int down = 1; down >= 0; down--) {
			SDL_zero(event);
			event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
			event.button.button = static_cast<Uint8>(click.button);
			event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
			event.button.clicks = 1;
			event.button.x = x;
			event.button.y = y;
			SDL_PushEvent(&event);
		}
	}
}

void TSdlGraphics::SaveShot(const std::string &path) {
	std::vector<unsigned char> pixels;
	int width = 0, height = 0;

	if (!ReadFrame(pixels, width, height))
		return;

	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(), width, height, 32, width * 4, SDL_PIXELFORMAT_ABGR8888);

	if (surface) {
		SDL_SaveBMP(surface, path.c_str());
		SDL_FreeSurface(surface);
	}
}

bool TSdlGraphics::ReadFrame(std::vector<unsigned char> &rgba, int &width, int &height) {
	if (!_renderer)
		return false;

	SDL_Rect none;

	SDL_GetWindowSize(VSPlayerWindow, &width, &height);
	SDL_RenderGetViewport(_renderer, &none);
	rgba.resize(static_cast<size_t>(width) * height * 4);
	return SDL_RenderReadPixels(_renderer, nullptr, SDL_PIXELFORMAT_ABGR8888, rgba.data(), width * 4) == 0;
}

#endif // USE_REAL_SDL

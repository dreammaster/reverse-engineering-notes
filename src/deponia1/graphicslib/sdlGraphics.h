// A graphics backend that draws with SDL2's renderer (SDL_Renderer: Direct3D, OpenGL or software, whichever the machine has), for the
// standalone player (USE_REAL_SDL). The original draws with its own OpenGL code (TGraphicsOGL, TGraphicsSubSystemGL, shaders), which is not
// reconstructed; this does what the game data needs of it: pictures with a tint, an alpha, a mirror, a turn and a scale, boxes and lines,
// all blended as premultiplied alpha (the pictures are premultiplied when they are decoded). What the data needs and this has not: nothing -
// it has no shader, particle emitter or matrix of the scripts (RENDER_EFFECTS of tests/deponia1/renderscene.cpp shows that the data has none).
// The game's picture is drawn into the part of the window that keeps its proportions (the rest is black), mouse positions are mapped back by
// g_displayedArea (ToScreenPos).
//
// The sprites are made in the thread that asks for them (the preloader's too): only the pixels are kept then, the texture is made when the
// sprite is first drawn (the renderer is used by the thread of the game only), and the textures of released sprites are destroyed there too.
#pragma once

#ifdef USE_REAL_SDL

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "SdlStub.h"
#include "graphicslib/graphics.h"
#include "graphicslib/graphicsBackend.h"

class TSdlGraphics : public TGraphicsInterface {
public:
	explicit TSdlGraphics(const TGraphicsStartup &startup);
	~TSdlGraphics() override;

	bool InitGraphics(const wxSize &windowSize, wxSize &renderSize, wxRect *displayedArea) override;
	void CalculateDisplayedArea(const wxSize &windowSize, const wxSize &renderSize, wxRect *displayedArea) override;
	void ToggleWindowMode() override;
	bool SetWindowSize(int width, int height) override;
	bool IsFullscreen() override;

	bool CreateSprite(TSpriteHandle **sprite, const char *data, int width, int height, int bytesPerPixel, int pitch,
	                  bool flag) override;
	void OnSpriteHandleReleased(TSpriteHandle *handle) override;

	void Draw(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
	          const unsigned int &color, int shader, float rotation, const wxPoint &rotationCenter, float scaleX, float scaleY,
	          int matrixId) override;
	void DrawBox(const wxRect &rect, unsigned int color, float alpha) override;
	void DrawLine(const wxPoint &from, const wxPoint &to, unsigned int color, float alpha) override;
	void SetOffset(float x, float y) override;
	void AfterDrawScene(bool a, bool b) override;
	void Swap() override;
	bool ReadScreen(std::vector<unsigned char> &rgb, int &width, int &height) override;

	/** What the window shows now, as the pixels of the window (RGBA); false when it cannot be read. */
	bool ReadFrame(std::vector<unsigned char> &rgba, int &width, int &height);
	/** DEPONIA_SHOTS=<directory>:<every>:<count> (the environment): a picture of the window (a .bmp file) is written for every `every`
	 *  frames shown, `count` of them: a way to see what the player draws without looking at the window. */
	void SaveShot(const std::string &path);
	/** DEPONIA_CLICKS=<frame>:<x>:<y>[:<button>];... (the environment): at the frame shown, a mouse click (button 1 left, 3 right) at the place of the game's
	 *  picture (x, y) is put in the queue of SDL events, as a mouse would: a way to test the player without a hand on the mouse. */
	void PushClicks();

private:
	struct Sprite {
		int width = 0;
		int height = 0;
		std::vector<unsigned char> pixels;   // until the texture is made
		SDL_Texture *texture = nullptr;
	};

	SDL_Texture *textureOf(Sprite &sprite);
	void destroyReleased();
	void fitToWindow();

	TGraphicsStartup _startup;
	SDL_Renderer *_renderer = nullptr;
	// The game is drawn into one of two textures of its own size (the pictures of the game's coordinates); at Swap() it is copied to the part of
	// the window that keeps the proportions, and the other one is drawn into next: what the last frame was stays to be read (the snapshots of the
	// savegames take it between two frames, when the picture of the window is not the one to read any more).
	SDL_Texture *_frames[2] = {nullptr, nullptr};
	int _current = 0;
	bool _hasLast = false;
	wxSize _renderSize;
	wxRect _area;
	int _windowWidth = 0;
	int _windowHeight = 0;
	float _offsetX = 0.0f;
	float _offsetY = 0.0f;
	bool _dirty = false;
	SDL_BlendMode _blend = SDL_BLENDMODE_NONE;
	bool _premultiplied = true;   // false: the renderer has no custom blend modes (SDL's software one): the pixels are made straight
	std::string _shotDirectory;
	int _shotEvery = 0;
	int _shotsLeft = 0;
	int _shown = 0;
	unsigned _fpsStart = 0;   // DEPONIA_FPS: the frames shown are counted and told every five seconds
	int _fpsFrames = 0;
	struct Click {
		int frame;
		int x;
		int y;
		int button;
	};
	std::vector<Click> _clicks;   // DEPONIA_CLICKS

	std::mutex _spritesLock;
	std::map<TSpriteHandle *, Sprite> _sprites;
	std::vector<SDL_Texture *> _released;
};

#endif // USE_REAL_SDL

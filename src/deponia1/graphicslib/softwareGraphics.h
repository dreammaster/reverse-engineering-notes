// A graphics backend that draws into a picture in memory, for running the game without a window (the tests, the checks of the
// real data): TGraphicsInterface with a software renderer in place of the OpenGL backend (TGraphicsOGL, not reconstructed).
// Sprites are kept as the RGBA pixels they were made from. A frame is built by Draw(), DrawBox() and DrawLine() and shown by
// Swap() (it is then the picture that GetFrame() gives and SavePNG() writes); AfterDrawScene(true, true) shows it too, which is what
// the transitions between scenes do for each of their frames (the fade out and in of the game, effect 3, are boxes of black with an
// alpha: the base class makes them and they come out as they are). What the data of Deponia 1 asks of a renderer is all here:
// `renderscene` with RENDER_EFFECTS=1 shows that it has no shader (the plain one is the only one), no particle emitter, no
// transition but the fade (effect 3) and that the scripts do not use the matrices; the light of a scene is the tint that
// TGScene::GetTint() gives an object (its light map sampled at the object), not a second texture. Not done, because nothing
// uses it: the shaders of a game, DrawWithLightMap() (it has no caller either in the original: the picture is drawn as it is),
// the particles, the matrices of the scripts, the dissolves and the iris of the other transition effects (4, 5, 6, 8, 9 and 11:
// FadeIn() plays their time without drawing).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "graphicslib/graphics.h"

class TSoftwareGraphics : public TGraphicsInterface {
public:
	TSoftwareGraphics(int width, int height);

	bool CreateSprite(TSpriteHandle **sprite, const char *data, int width, int height, int bytesPerPixel, int pitch,
	                  bool flag) override;
	void OnSpriteHandleReleased(TSpriteHandle *handle) override;

	void Draw(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
	          const unsigned int &color, int shader, float rotation, const wxPoint &rotationCenter, float scaleX, float scaleY,
	          int matrixId) override;
	void DrawWithLightMap(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
	                      void *lightMap, const wxPoint &offset) override;
	void DrawBox(const wxRect &rect, unsigned int color, float alpha) override;
	void DrawLine(const wxPoint &from, const wxPoint &to, unsigned int color, float alpha) override;
	void SetOffset(float x, float y) override;
	void AfterDrawScene(bool a, bool b) override;
	void Swap() override;
	bool ReadScreen(std::vector<unsigned char> &rgb, int &width, int &height) override;

	int GetFrameWidth() const {
		return _frameWidth;
	}
	int GetFrameHeight() const {
		return _frameHeight;
	}
	/** The frame that was shown last (Swap()), RGBA, `GetFrameWidth()` pixels to a line. */
	const std::vector<uint8_t> &GetFrame() const {
		return _shown;
	}
	/** The frame so far (what is drawn since the last Swap()). */
	const std::vector<uint8_t> &GetCurrentFrame() const {
		return _frame;
	}
	/** How many pictures were drawn in the last frame that was shown. */
	int GetDrawnCount() const {
		return _shownDrawn;
	}
	/** Without rendering the sprites are kept and the draw calls are counted but no pixels are made (a run that only plays the game). */
	void SetRendering(bool rendering) {
		_rendering = rendering;
	}
	/** Called when a frame is shown (the frames of a transition too, which are drawn inside one call of the game). */
	void SetShownCallback(const std::function<void(const TSoftwareGraphics &)> &callback) {
		_onShown = callback;
	}
	/** Writes the last frame that was shown as a PNG, made `reduce` times smaller (1: as it is). */
	bool SavePNG(const char *path, int reduce = 1) const;

private:
	struct Pixels {
		int width = 0;
		int height = 0;
		std::vector<uint8_t> rgba;
	};

	void blend(int x, int y, float r, float g, float b, float a);

	int _frameWidth;
	int _frameHeight;
	std::vector<uint8_t> _frame;
	std::vector<uint8_t> _shown;
	std::map<TSpriteHandle *, Pixels> _sprites;
	float _offsetX = 0.0f;
	float _offsetY = 0.0f;
	int _drawn = 0;
	bool _dirty = false;
	bool _rendering = true;
	std::function<void(const TSoftwareGraphics &)> _onShown;
	int _shownDrawn = 0;
};

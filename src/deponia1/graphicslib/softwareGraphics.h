// A graphics backend that draws into a picture in memory, for running the game without a window (the tests, the checks of the
// real data): TGraphicsInterface with a software renderer in place of the OpenGL backend (TGraphicsOGL, not reconstructed).
// Sprites are kept as the RGBA pixels they were made from. A frame is built by Draw(), DrawBox() and DrawLine() and shown by
// Swap() (it is then the picture that GetFrame() gives and SavePNG() writes). Not done: the shaders (a picture is drawn
// with the plain shader), the light map of DrawWithLightMap (the picture is drawn as it is), the particles, the matrices of the
// scripts, the transitions between scenes (FadeIn() plays its time without drawing).
#pragma once

#include <cstdint>
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
	void Swap() override;

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
	int _shownDrawn = 0;
};

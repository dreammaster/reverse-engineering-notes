#pragma once
// 320x200 16-colour frame buffer and the main game screen composition (KEH windows, borders, map view).
#include "fod/gfx.h"
#include "fod/world.h"

namespace fod {

class Screen {
public:
	static constexpr int W = 320, H = 200;
	Screen() : _px(size_t(W) * H, 0) {}
	void clear(uint8_t c = 0) { std::fill(_px.begin(), _px.end(), c); }
	void put(int x, int y, uint8_t c) { if (x >= 0 && y >= 0 && x < W && y < H) _px[size_t(y) * W + x] = c; }
	uint8_t get(int x, int y) const { return _px[size_t(y) * W + x]; }
	Image toImage() const;
	const Bytes &pixels() const { return _px; }
private:
	Bytes _px;
};

// KEH picture set (blitPicFrame): byte count, 2 bytes, then u16 offsets (from the set start) at +3. Each frame is
// {widthBytes, height, x, y} followed by 4bpp rows; x is added to the blit position in pixels.
class PictureSet {
public:
	explicit PictureSet(Bytes d) : _d(std::move(d)) {}
	size_t count() const { return _d.empty() ? 0 : _d[0]; }
	void blit(Screen &s, int frame, int x, int y) const;   // x,y: origin the frame offsets are relative to
	int frameWidth(int frame) const;                       // pixels
	int frameHeight(int frame) const;
private:
	size_t frameOffset(int frame) const;
	Bytes _d;
};

// Static assets needed to draw the game screens.
class Assets {
public:
	explicit Assets(const std::string &gameDir);
	Bytes tiles;     // 1000 * 128 bytes, 4bpp
	Bytes font;      // 209 * 32 bytes
	Bytes borders;   // 6000 bytes
};

void drawTile(Screen &s, const Assets &a, int x, int y, int tile);
void drawGlyph(Screen &s, const Assets &a, int x, int y, int ch, bool inverse = false);
void drawText(Screen &s, const Assets &a, int col, int row, const std::string &text, bool inverse = false);
void drawBorders(Screen &s, const Assets &a, int variant);          // 0 main, 1 char sheet, 2.., 3, 4
void drawMapView(Screen &s, const Assets &a, const World &w);       // 19x9 tiles at (8,24)
void drawPartyNames(Screen &s, const Assets &a, const World &w);
void drawClock(Screen &s, const Assets &a, const World &w);

// Full main screen, with the message window showing `messages` (last lines).
void renderMainScreen(Screen &s, const Assets &a, const World &w, const std::vector<std::string> &messages);

} // namespace fod

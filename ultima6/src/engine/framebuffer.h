#ifndef ULTIMA6_ENGINE_FRAMEBUFFER_H
#define ULTIMA6_ENGINE_FRAMEBUFFER_H

#include <cstdint>
#include <string>
#include <vector>

namespace Ultima6 {

/**
 * The 320 x 200, 256-colour screen all the games draw on (VGA mode 13h), with its palette
 * (6 bits per component, as the games store it).
 */
class Framebuffer {
public:
	enum { WIDTH = 320, HEIGHT = 200 };

	Framebuffer() : _pixels(WIDTH * HEIGHT, 0) {
		for (int i = 0; i < 256 * 3; ++i)
			_palette[i] = 0;
	}

	uint8_t *pixels() {
		return &_pixels[0];
	}
	const uint8_t *pixels() const {
		return &_pixels[0];
	}
	void setPalette(const uint8_t rgb6[256][3]);
	/** 8-bit RGB of a colour. */
	void colour(int index, int &r, int &g, int &b) const;

	void clear(uint8_t value) {
		for (uint8_t &p : _pixels)
			p = value;
	}
	void fillRect(int x, int y, int w, int h, uint8_t value);
	/** Copies w * h source pixels (row stride srcPitch); pixels equal to `transparent` (if >= 0) are skipped. Clipped. */
	void blit(int x, int y, const uint8_t *src, int w, int h, int srcPitch, int transparent = -1);

	/** Writes the screen as a PNG (scaled by `scale`); false on failure. */
	bool savePng(const std::string &path, int scale = 1) const;

private:
	std::vector<uint8_t> _pixels;
	uint8_t _palette[256 * 3];
};

} // namespace Ultima6

#endif

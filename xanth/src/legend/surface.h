/* An 8-bit indexed picture with a palette: the 320x200 VGA screen of the original, and off-screen buffers.
 *
 * Everything drawn by the game is palette indices; the palette (6-bit VGA components) is applied only when the surface is converted to RGB for display or for a
 * screenshot, so fades and palette cycling stay cheap. Coordinates are clipped to the surface.
 */
#ifndef LEGEND_SURFACE_H
#define LEGEND_SURFACE_H

#include "common/types.h"

namespace Legend {

class Surface {
public:
	Surface(int width = 320, int height = 200);

	int width() const { return _w; }
	int height() const { return _h; }
	byte *pixels() { return &_pix[0]; }
	const byte *pixels() const { return &_pix[0]; }
	byte get(int x, int y) const { return x >= 0 && y >= 0 && x < _w && y < _h ? _pix[(size_t)y * _w + x] : 0; }
	void put(int x, int y, byte c) {
		if (x >= 0 && y >= 0 && x < _w && y < _h)
			_pix[(size_t)y * _w + x] = c;
	}

	void clear(byte color = 0);
	void fillRect(const Common::Rect &r, byte color);
	void frameRect(const Common::Rect &r, byte color);
	/** Copies a block of palette indices; index `transparent` (if >= 0) is skipped. */
	void blit(int x, int y, const byte *src, int srcWidth, int srcHeight, int transparent = -1);
	/** Copies part of another surface. */
	void copyFrom(const Surface &src, const Common::Rect &srcRect, int x, int y);

	/** 6-bit VGA palette, 256 * 3 bytes. */
	byte palette[768];
	void setPalette(const byte *vga768);
	/** RGB (3 bytes per pixel) for display; `rgb` must hold width * height * 3 bytes. */
	void toRgb(byte *rgb) const;
	/** Writes a binary PPM (for tests and screenshots). */
	bool savePpm(const std::string &path) const;

private:
	int _w, _h;
	std::vector<byte> _pix;
};

} // namespace Legend

#endif

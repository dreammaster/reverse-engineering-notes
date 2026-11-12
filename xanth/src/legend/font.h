/* XANTH_nn.FNT: a bitmap font.
 *
 * Layout (verified on all fonts of Xanth and Shannara): 10 header bytes [0] ?  [1] bits per pixel (1 = monochrome, 8 = colour glyphs with 0xFE transparent)
 * [2] bytes per glyph row  [3] first character  [4] last character  [5] line spacing  [6] glyph height  [7] glyph width (0x80 and up: a width table follows)
 * [8] character spacing (0x80 and up: a table of packed nibbles follows, high = space before the glyph, low = space after)  [9] ?; then u8 widths[last + 1] if proportional, u8 spacing[last + 1] if per character, then the glyphs:
 * (last - first + 1) * bytesPerRow * height bytes, 1 bpp most significant bit first (8 bpp: one byte per pixel).
 */
#ifndef LEGEND_FONT_H
#define LEGEND_FONT_H

#include "common/types.h"

namespace Legend {

class Surface;

class Font {
public:
	/** False if the data is not a font of this layout. */
	bool load(const Common::Bytes &data);

	int height() const { return _height; }
	int lineSpacing() const { return _lineSpacing; }
	int firstChar() const { return _first; }
	int lastChar() const { return _last; }
	bool hasGlyph(int ch) const { return ch >= _first && ch <= _last; }
	/** Pixel width of the glyph. */
	int charWidth(int ch) const;
	/** Pixels the pen moves right before the glyph is drawn (the high nibble of a per-character spacing byte) and the space after it (the low nibble, or the fixed spacing). */
	int lead(int ch) const;
	int trail(int ch) const;
	/** Pixels the pen advances in all: lead + width + trail (Font_CharWidth of the original, without its +1 in overprint mode). */
	int advance(int ch) const;
	int textWidth(const std::string &text) const;

	/** Draws one glyph with its top-left corner at (x, y) in `color` (monochrome fonts) or in its own colours (8 bpp fonts); returns the advance. */
	int drawChar(Surface &dst, int x, int y, int ch, byte color) const;
	int drawText(Surface &dst, int x, int y, const std::string &text, byte color) const;

private:
	int _bpp, _bytesPerRow, _first, _last, _lineSpacing, _height, _width, _spacing;
	std::vector<byte> _widths, _spacings, _glyphs;
};

} // namespace Legend

#endif

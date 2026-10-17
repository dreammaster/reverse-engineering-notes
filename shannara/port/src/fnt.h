// SHANxxx.FNT bitmap fonts (size formula verified on all 13 retail fonts).
//   10 header bytes: [1] bits per pixel (1 mono, 8 colour: 0xFE transparent) [2] bytesPerRow [3] firstChar [4] lastChar
//   [5] lineSpacing [6] charHeight [7] charWidth (>=0x80: proportional, table follows) [8] charSpacing (>=0x80: table follows)
//   then u8 widths[last+1] if proportional, u8 spacing[last+1] if per-char, then (last-first+1)*bytesPerRow*charHeight glyph bytes
#pragma once
#include "common.h"

namespace shannara {

class Font {
public:
	explicit Font(Bytes data);
	int firstChar() const { return _first; }
	int lastChar() const { return _last; }
	int height() const { return _height; }
	int lineSpacing() const { return _lineSpacing; }
	int bitsPerPixel() const { return _bpp; }
	int bytesPerRow() const { return _bpr; }
	int width(int ch) const { return _widths.empty() ? _cw : _widths[ch]; }
	// glyph bitmap (bytesPerRow * height bytes) or nullptr if the character is not in the font
	const uint8_t *glyph(int ch) const;
	size_t expectedSize() const { return _expected; }
	size_t actualSize() const { return _data.size(); }
private:
	Bytes _data;
	int _bpp, _bpr, _first, _last, _lineSpacing, _height, _cw, _cs;
	Bytes _widths, _spacing;
	size_t _glyphBase = 0, _expected = 0;
};

} // namespace shannara

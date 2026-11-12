#include "legend/font.h"

#include "legend/surface.h"

namespace Legend {

bool Font::load(const Common::Bytes &d) {
	if (d.size() < 10)
		return false;
	_bpp = d[1];
	_bytesPerRow = d[2];
	_first = d[3];
	_last = d[4];
	_lineSpacing = d[5];
	_height = d[6];
	_width = d[7];
	_spacing = d[8];
	if ((_bpp != 1 && _bpp != 8) || _last < _first || _bytesPerRow == 0 || _height == 0)
		return false;
	size_t pos = 10;
	_widths.clear();
	_spacings.clear();
	if (_width >= 0x80) {
		if (d.size() < pos + _last + 1u)
			return false;
		_widths.assign(d.begin() + pos, d.begin() + pos + _last + 1);
		pos += _last + 1;
	}
	if (_spacing >= 0x80) {
		if (d.size() < pos + _last + 1u)
			return false;
		_spacings.assign(d.begin() + pos, d.begin() + pos + _last + 1);
		pos += _last + 1;
	}
	size_t glyphBytes = (size_t)(_last - _first + 1) * _bytesPerRow * _height;
	if (d.size() < pos + glyphBytes)
		return false;
	_glyphs.assign(d.begin() + pos, d.begin() + pos + glyphBytes);
	return true;
}

int Font::charWidth(int ch) const {
	if (_widths.empty())
		return _width;
	return ch >= 0 && ch < (int)_widths.size() ? _widths[ch] : 0;
}

int Font::advance(int ch) const {
	return charWidth(ch) + lead(ch) + trail(ch);
}

int Font::lead(int ch) const {
	return _spacings.empty() ? 0 : (ch >= 0 && ch < (int)_spacings.size() ? _spacings[ch] >> 4 : 0);
}

int Font::trail(int ch) const {
	if (_spacings.empty())
		return _spacing;
	return ch >= 0 && ch < (int)_spacings.size() ? _spacings[ch] & 15 : 0;
}

int Font::textWidth(const std::string &text) const {
	int w = 0;
	for (size_t i = 0; i < text.size(); i++)
		w += advance((byte)text[i]);
	return w;
}

int Font::drawChar(Surface &dst, int x, int y, int ch, byte color) const {
	if (hasGlyph(ch)) {
		x += lead(ch);
		const byte *g = &_glyphs[(size_t)(ch - _first) * _bytesPerRow * _height];
		int w = charWidth(ch);
		for (int row = 0; row < _height; row++) {
			const byte *r = g + (size_t)row * _bytesPerRow;
			for (int col = 0; col < w; col++) {
				if (_bpp == 8) {
					byte v = col < _bytesPerRow ? r[col] : 0;
					if (v != 0 && v != 0xFE)
						dst.put(x + col, y + row, v);
				} else if (r[col >> 3] & (0x80 >> (col & 7))) {
					dst.put(x + col, y + row, color);
				}
			}
		}
	}
	return advance(ch);
}

int Font::drawText(Surface &dst, int x, int y, const std::string &text, byte color) const {
	int start = x;
	for (size_t i = 0; i < text.size(); i++)
		x += drawChar(dst, x, y, (byte)text[i], color);
	return x - start;
}

} // namespace Legend

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

int Font::drawChar(Surface &dst, int x, int y, int ch, byte color, int background) const {
	if (background >= 0) {
		for (int row = 0; row < lineHeight(); row++)
			for (int col = 0; col < advance(ch); col++)
				dst.put(x + col, y + row, (byte)background);
	}
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

int Font::drawText(Surface &dst, int x, int y, const std::string &text, byte color, int background) const {
	int start = x;
	for (size_t i = 0; i < text.size(); i++)
		x += drawChar(dst, x, y, (byte)text[i], color, background);
	return x - start;
}

bool Font::glyphBit(int ch, int col, int row) const {
	if (!hasGlyph(ch) || col < 0 || row < 0 || row >= _height || col >= charWidth(ch))
		return false;
	const byte *g = &_glyphs[(size_t)(ch - _first) * _bytesPerRow * _height];
	return (g[(size_t)row * _bytesPerRow + (col >> 3)] & (0x80 >> (col & 7))) != 0;
}

int Font::drawCharOutlined(Surface &dst, int x, int y, int ch, byte color, byte outline) const {
	x += lead(ch);
	int w = charWidth(ch);
	for (int cy = 0; cy < _height + 2; cy++) {
		for (int cx = 0; cx < w + 2; cx++) {
			int gx = cx - 1, gy = cy - 1;
			if (glyphBit(ch, gx, gy)) {
				dst.put(x + cx, y + cy, color);
				continue;
			}
			bool near = false;
			for (int dy = -1; dy <= 1 && !near; dy++)
				for (int dx = -1; dx <= 1 && !near; dx++)
					near = glyphBit(ch, gx + dx, gy + dy);
			if (near)
				dst.put(x + cx, y + cy, outline);
		}
	}
	return advance(ch) + 1;
}

int Font::drawTextOutlined(Surface &dst, int x, int y, const std::string &text, byte color, byte outline) const {
	int start = x;
	for (size_t i = 0; i < text.size(); i++)
		x += drawCharOutlined(dst, x, y, (byte)text[i], color, outline);
	return x - start;
}

} // namespace Legend

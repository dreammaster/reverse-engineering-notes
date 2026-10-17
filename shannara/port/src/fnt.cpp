#include "fnt.h"

namespace shannara {

Font::Font(Bytes data) : _data(std::move(data)) {
	Cursor c(_data);
	c.u8();
	_bpp = c.u8();
	_bpr = c.u8();
	_first = c.u8();
	_last = c.u8();
	_lineSpacing = c.u8();
	_height = c.u8();
	_cw = c.u8();
	_cs = c.u8();
	c.u8();
	int n = _last + 1;
	if (_cw >= 0x80)
		for (int i = 0; i < n; i++)
			_widths.push_back(c.u8());
	if (_cs >= 0x80)
		for (int i = 0; i < n; i++)
			_spacing.push_back(c.u8());
	_glyphBase = c.pos();
	_expected = _glyphBase + (size_t)(_last - _first + 1) * _bpr * _height;
	if (_expected > _data.size())
		throw FormatError("font data too short");
}

const uint8_t *Font::glyph(int ch) const {
	if (ch < _first || ch > _last)
		return nullptr;
	return &_data[_glyphBase + (size_t)(ch - _first) * _bpr * _height];
}

} // namespace shannara

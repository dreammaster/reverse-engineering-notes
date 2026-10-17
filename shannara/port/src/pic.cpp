#include "pic.h"

namespace shannara {

PicFile::PicFile(Bytes data) : _data(std::move(data)) {
	for (size_t i = 0; i * 12 + 12 <= _data.size(); i++) {
		Cursor c(_data, i * 12);
		PicEntry e;
		e.offset = c.u32();
		if (e.offset == 0)
			break;
		e.index = (int)i;
		e.frames = c.u8();
		e.flags = c.u8();
		e.w = c.u16();
		e.h = c.u16();
		e.b10 = c.u8();
		e.b11 = c.u8();
		_entries.push_back(e);
		if ((i + 1) * 12 >= _entries[0].offset) // directory ends where the first picture starts
			break;
	}
	for (size_t k = 0; k < _entries.size(); k++)
		_entries[k].end = k + 1 < _entries.size() ? _entries[k + 1].offset : (uint32_t)_data.size();
}

PicImage PicFile::load(int entry) const {
	if (entry < 0 || entry >= (int)_entries.size())
		throw FormatError("bad picture entry");
	const PicEntry &e = _entries[entry];
	Cursor c(_data, e.offset);
	PicImage img;
	for (int i = 0; i < e.frames; i++) {
		PicImage::Frame f;
		f.x = c.s16();
		f.y = c.s16();
		img.offsets.push_back(f);
	}
	if (e.hasPalette()) {
		img.hasPalette = true;
		for (int i = 0; i < 768; i++)
			img.palette[i] = c.u8();
	}
	size_t n = (size_t)e.w * e.h * (e.frames ? e.frames : 1);
	if (c.pos() + n > _data.size())
		throw FormatError("picture data past end of file");
	img.pixels.assign(c.ptr(), c.ptr() + n);
	return img;
}

void PicFile::compositeXor(Bytes &bg, int bgWidth, const uint8_t *spr, int sx, int sy, int sw, int sh) {
	for (int y = 0; y < sh; y++)
		for (int x = 0; x < sw; x++)
			bg[(size_t)(sy + y) * bgWidth + sx + x] ^= spr[y * sw + x];
}

} // namespace shannara

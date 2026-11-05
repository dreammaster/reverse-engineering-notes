#include "legend/pic.h"

#include "common/stream.h"
#include "legend/dcl.h"

namespace Legend {

bool PicFile::load(const Common::Bytes &data, bool compressed) {
	_d = data;
	_compressed = compressed;
	_entries.clear();
	Common::ReadStream s(_d);
	for (size_t i = 0; (i + 1) * 12 <= _d.size(); i++) {
		s.seek(i * 12);
		Entry e;
		e.offset = s.readUint32LE();
		if (e.offset == 0)
			break;
		e.frames = s.readByte();
		e.flags = s.readByte();
		e.width = s.readUint16LE();
		e.height = s.readUint16LE();
		e.end = 0;
		_entries.push_back(e);
		if ((i + 1) * 12 >= _entries[0].offset)         // the directory ends where the first picture's data starts
			break;
	}
	if (_entries.empty())
		return false;
	for (size_t k = 0; k < _entries.size(); k++)
		_entries[k].end = k + 1 < _entries.size() ? _entries[k + 1].offset : (uint32)_d.size();
	return !s.err();
}

bool PicFile::decode(size_t i, PicImage &out) const {
	if (i >= _entries.size() || _entries[i].offset == 0)
		return false;
	const Entry &e = _entries[i];
	out = PicImage();
	out.width = e.width;
	out.height = e.height;
	out.flags = e.flags;
	out.frames = e.frames;
	Common::ReadStream s(_d);
	s.seek(e.offset);
	for (unsigned f = 0; f < e.frames; f++) {
		int16 x = s.readSint16LE(), y = s.readSint16LE();
		out.framePos.push_back(Common::Point(x, y));
	}
	if (e.frames) {
		out.x = out.framePos[0].x;
		out.y = out.framePos[0].y;
	}
	if (e.flags & 0x10) {
		out.hasPalette = s.read(out.palette, sizeof(out.palette));
		if (!out.hasPalette)
			return false;
	}
	if (s.err() || s.pos() > e.end)
		return false;
	size_t want = (size_t)e.width * e.height;
	if (_compressed) {
		if (!dclExplode(_d.empty() ? nullptr : &_d[s.pos()], e.end - s.pos(), out.pixels))
			return false;
		return out.pixels.size() == want;
	}
	if (s.pos() + want > _d.size())
		return false;
	out.pixels.assign(_d.begin() + s.pos(), _d.begin() + s.pos() + want);
	return true;
}

} // namespace Legend

#include "mus.h"
#include <cstring>

namespace shannara {

MusFile::MusFile(Bytes data) : _data(std::move(data)) {
	Cursor c(_data);
	int count = c.u16();
	for (int i = 0; i < count; i++) {
		MusTrack t;
		t.offset = c.u32();
		t.size = c.u16();
		if (t.size == 0)
			continue; // terminator entry
		if ((size_t)t.offset + t.size > _data.size())
			throw FormatError("music track past end of file");
		const uint8_t *p = &_data[t.offset];
		// FORM(XDIR){INFO} at 0, CAT(XMID) at 22: size == 22 + 8 + big-endian CAT length
		uint32_t cat = t.size >= 34 ? ((uint32_t)p[26] << 24) | (p[27] << 16) | (p[28] << 8) | p[29] : 0;
		t.validXmidi = t.size >= 34 && !memcmp(p, "FORM", 4) && !memcmp(p + 8, "XDIR", 4) && !memcmp(p + 22, "CAT ", 4) &&
		               !memcmp(p + 30, "XMID", 4) && (uint32_t)t.size == 30 + cat;
		_tracks.push_back(t);
	}
}

} // namespace shannara

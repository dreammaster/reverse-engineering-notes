#include "legend/audiofiles.h"

#include <algorithm>

#include "common/stream.h"

namespace Legend {

// ---- MUS: 12-byte directory {u32 offset; u16 size, a, b, c}; every record is a standard MIDI file ----------------------------------------------------------------

bool MusFile::load(const Common::Bytes &data) {
	_d = data;
	_records.clear();
	Common::ReadStream s(_d);
	uint32 first = s.readUint32LE();
	if (first < 12 || first > _d.size())
		return false;
	for (uint32 i = 0; i < first / 12; i++) {
		s.seek(i * 12);
		Record r;
		r.offset = s.readUint32LE();
		r.size = s.readUint16LE();
		r.a = s.readUint16LE();
		r.b = s.readUint16LE();
		r.c = s.readUint16LE();
		_records.push_back(r);
	}
	return !s.err();
}

const MusFile::Record *MusFile::tune(unsigned t, unsigned arrangement) const {
	size_t i = 2 * (size_t)t - 2 + (arrangement & 1);
	if (t == 0 || i >= _records.size() || _records[i].offset == 0 || _records[i].offset >= _d.size())
		return nullptr;
	return &_records[i];
}

size_t MusFile::smfLength(const Common::Bytes &d, size_t off) {
	if (off + 14 > d.size() || memcmp(&d[off], "MThd", 4) != 0)
		return 0;
	Common::ReadStream s(d);
	s.seek(off + 4);
	size_t p = off + 8 + s.readUint32BE();
	s.seek(off + 10);
	unsigned ntr = s.readUint16BE();
	for (unsigned i = 0; i < ntr; i++) {
		if (p + 8 > d.size() || memcmp(&d[p], "MTrk", 4) != 0)
			return 0;
		s.seek(p + 4);
		p += 8 + s.readUint32BE();
	}
	return p > d.size() ? 0 : p - off;
}

bool MusFile::midi(const Record &r, Common::Bytes &out) const {
	size_t n = smfLength(_d, r.offset);
	if (!n)
		return false;
	out.assign(_d.begin() + r.offset, _d.begin() + r.offset + n);
	return true;
}

// ---- VOC: u16 count; count * u32 absolute offsets (the last is the file size) -----------------------------------------------------------------------------------

bool VocFile::load(const Common::Bytes &dir) {
	_offsets.clear();
	Common::ReadStream s(dir);
	unsigned n = s.readUint16LE();
	for (unsigned i = 0; i < n; i++)
		_offsets.push_back(s.readUint32LE());
	return !s.err() && _offsets.size() >= 2;
}

size_t VocFile::trimmedSize(const byte *samples, size_t size) {
	while (size && samples[size - 1] == 0x80)
		size--;
	return size;
}

// ---- RS: "STEVE", kind, scale, u32 length at +7, samples from +0x20 -----------------------------------------------------------------------------------------------

bool RsFile::load(const Common::Bytes &d) {
	_samples.clear();
	if (d.size() < 32 || memcmp(&d[0], "STEVE", 5) != 0)
		return false;
	Common::ReadStream s(d);
	s.seek(5);
	_kind = s.readByte();
	_scale = s.readByte();
	uint32 len = s.readUint32LE();
	if (!_scale || 32 + (size_t)len > d.size())
		return false;
	_samples.assign(d.begin() + 32, d.begin() + 32 + len);
	return true;
}

Common::Bytes RsFile::toPcm8() const {
	Common::Bytes out(_samples.size());
	for (size_t i = 0; i < out.size(); i++)
		out[i] = (byte)std::min<unsigned>(255, _samples[i] * 255u / _scale);
	return out;
}

} // namespace Legend

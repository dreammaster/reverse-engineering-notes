// SHAN001.MUS music archive (and the demo's Demo001.mus): u16 count, then count entries {u32 offset, u16 size}; the last
// entry is a terminator (offset = file size, size 0). Every track is a complete XMIDI file ("FORM....XDIR" ... "CAT XMID").
#pragma once
#include "common.h"

namespace shannara {

struct MusTrack {
	uint32_t offset = 0;
	uint16_t size = 0;
	bool validXmidi = false; // starts with FORM/XDIR and its size matches the CAT chunk
};

class MusFile {
public:
	explicit MusFile(Bytes data);
	int trackCount() const { return (int)_tracks.size(); }
	const MusTrack &track(int i) const { return _tracks[i]; }
	// pointer to the XMIDI bytes of a track (size() bytes)
	const uint8_t *data(int i) const { return &_data[_tracks[i].offset]; }
private:
	Bytes _data;
	std::vector<MusTrack> _tracks;
};

} // namespace shannara

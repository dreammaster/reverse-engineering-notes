/* The audio data files of Companions of Xanth.
 *
 *   MusFile   XANTH_nn.MUS  an archive of standard MIDI files (two arrangements per tune: slot 2t-2 and 2t-1, chosen by the sound device)
 *   VocFile   XANTH_01.VOC  the CD version's speech: a directory of 8-bit unsigned PCM tracks, track N = recording of conversation line N (the sample rate is not stored;
 *                           about 22050 Hz was measured). The file is ~190 MB, so only its directory is loaded and tracks are read on demand.
 *   RsFile    *.RS          RealSound PC-speaker clips: a 32-byte "STEVE" header then unsigned PWM samples whose full scale is header[6] (the PIT period; the sample rate
 *                           is 1193182 / full scale = about 16572 Hz)
 */
#ifndef LEGEND_AUDIOFILES_H
#define LEGEND_AUDIOFILES_H

#include "common/types.h"

namespace Legend {

class MusFile {
public:
	struct Record {
		uint32 offset;
		uint16 size, a, b, c;
	};

	bool load(const Common::Bytes &data);
	/** All directory slots (120 in the shipped files; unused slots have offset 0 or filler). */
	const std::vector<Record> &records() const { return _records; }
	/** Tune t (1-based) in one of its two arrangements, or nullptr if the slot is empty. */
	const Record *tune(unsigned t, unsigned arrangement) const;
	/** The standard MIDI file of a record (the second arrangement stores 4 extra bytes after the file, which are not returned); false if it is not a MIDI file. */
	bool midi(const Record &r, Common::Bytes &out) const;
	static size_t smfLength(const Common::Bytes &d, size_t off);   // 0 if there is no MIDI file at off

private:
	Common::Bytes _d;
	std::vector<Record> _records;
};

class VocFile {
public:
	/** `directory` needs the first 2 + 4 * count bytes of the file (the offsets of the tracks, the last is the file size). */
	bool load(const Common::Bytes &directory);
	size_t trackCount() const { return _offsets.empty() ? 0 : _offsets.size() - 1; }
	bool hasTrack(size_t n) const { return n < trackCount() && _offsets[n + 1] > _offsets[n]; }
	uint32 trackOffset(size_t n) const { return _offsets[n]; }
	uint32 trackSize(size_t n) const { return _offsets[n + 1] - _offsets[n]; }
	/** Length of a track's samples without the 0x80 (silence) padding up to the next 4 KB boundary. */
	static size_t trimmedSize(const byte *samples, size_t size);

private:
	std::vector<uint32> _offsets;
};

class RsFile {
public:
	bool load(const Common::Bytes &data);
	byte fullScale() const { return _scale; }
	const Common::Bytes &samples() const { return _samples; }
	unsigned sampleRate() const { return _scale ? 1193182u / _scale : 0; }
	/** Unsigned 8-bit PCM scaled to the full 0..255 range. */
	Common::Bytes toPcm8() const;

private:
	byte _kind = 0, _scale = 0;
	Common::Bytes _samples;
};

} // namespace Legend

#endif

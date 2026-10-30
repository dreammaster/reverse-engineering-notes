/* Readers for the Legend Entertainment "Later engine" data formats used by Companions of Xanth (and, except where noted, Shannara).
 *
 * Every reader here is a C++ port of a Python reference in xanth/reversing/scripts (or shannara/reversing/scripts) that was verified against the
 * game files; tools/legend_tool.cpp checks them against the same data.  No dependencies beyond the C++17 standard library: the intent is that the
 * code can be dropped into a ScummVM engine (Common::SeekableReadStream wrappers around the byte-vector interfaces are trivial).
 *
 *   DclExplode   PKWARE DCL "implode" stream (Xanth .PIC pixel data)
 *   PicFile      XANTH_nn.PIC picture container
 *   RgnFile      XANTH_nn.RGN hotspot regions (region id == object id)
 *   MessageFile  XANTHSTR.DAT / SHANSTR.DAT Huffman-coded message blocks
 *   MusFile      XANTH_nn.MUS archive of standard MIDI files
 *   VocFile      XANTH_01.VOC voice track directory (track N == conversation line N)
 */
#ifndef LEGEND_FORMATS_H
#define LEGEND_FORMATS_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace Legend {

typedef std::vector<uint8_t> Bytes;

struct FormatError : std::runtime_error {
	explicit FormatError(const std::string &m) : std::runtime_error(m) {}
};

Bytes readFile(const std::string &path);

/** Decodes a PKWARE DCL implode stream (blast.c algorithm).  `consumed` (optional) receives the number of input bytes used. */
Bytes dclExplode(const uint8_t *data, size_t size, size_t *consumed = nullptr);

/** One 6-bit VGA palette entry set (768 bytes); use palette8() to expand a component to 8 bits. */
inline uint8_t palette8(uint8_t v6) { v6 &= 63; return (uint8_t)(v6 * 4 + (v6 >> 4)); }

class PicFile {
public:
	struct Entry {
		uint32_t offset;     // start of the entry's data
		uint32_t end;        // start of the next entry (or the file size)
		uint8_t  count;      // number of {i16 x, i16 y} offsets before the data
		uint8_t  flags;      // 0x10 = palette present, 0x20/0x40 = pixel/overlay flags; Xanth compresses all pictures with DCL
		uint16_t width, height;
	};
	struct Image {
		int16_t x = 0, y = 0;     // screen offset of the picture
		uint16_t width = 0, height = 0;
		bool hasPalette = false;
		uint8_t palette[768];     // 6-bit VGA components
		Bytes pixels;             // width*height palette indices
	};
	explicit PicFile(Bytes data);
	size_t size() const { return _entries.size(); }
	const Entry &entry(size_t i) const { return _entries.at(i); }
	/** Decodes entry i. Xanth's pixel data is DCL-compressed; Shannara's retail .PIC is stored raw (compressed == false). */
	Image decode(size_t i, bool compressed = true) const;
private:
	Bytes _d;
	std::vector<Entry> _entries;
};

class RgnFile {
public:
	struct Region {
		int16_t id;                   // Xanth: the object id of the thing under the hotspot
		bool    rect;                 // true: pts = {x1, y1, x2, y2}; false: polygon, pts = x,y pairs
		std::vector<int16_t> pts;
		bool contains(int x, int y) const;
	};
	struct Scene {
		uint16_t baseId = 0;          // Xanth: the room's object id
		std::vector<Region> regions;  // drawn order == lookup order (first hit wins)
	};
	explicit RgnFile(const Bytes &data);
	size_t size() const { return _scenes.size(); }
	const Scene &scene(size_t i) const { return _scenes.at(i); }
private:
	std::vector<Scene> _scenes;       // one per entry of the same-numbered .PIC; empty when the dataOffset is 0
};

class MessageFile {
public:
	explicit MessageFile(Bytes data);
	size_t blockCount() const { return _blocks.size(); }
	size_t stringCount(size_t block) const { return _blocks.at(block).lengths.size(); }
	/** Decoded string (latin-1, includes embedded control codes like 0x0B and '\n'; the trailing NUL is removed). */
	std::string string(size_t block, size_t index) const;
	/** Message ids in the object code are 0xF0BBIIII: block BB, index IIII. */
	std::string string(uint32_t messageId) const { return string((messageId >> 16) & 0xFF, messageId & 0xFFFF); }
private:
	struct Block { uint32_t dataOffset; std::vector<uint16_t> lengths; std::vector<uint32_t> offsets; };
	Bytes _d;
	std::vector<Block> _blocks;
	std::vector<int16_t> _tree;
	std::vector<std::string> _phrases;
};

class MusFile {
public:
	struct Record { uint32_t offset; uint16_t size, a, b, c; };
	explicit MusFile(Bytes data);
	/** All directory slots (120 in the shipped files; unused slots have offset 0 or 0xFFFF filler). */
	const std::vector<Record> &records() const { return _records; }
	/** Tune t (1-based, per the music table musicId & 0xFF) has two MIDI arrangements: slot 2t-2 and 2t-1 (device dependent). */
	const Record *tune(unsigned t, unsigned arrangement) const;
	/** Returns the standard MIDI file bytes of a record (the odd/second arrangement stores 4 extra bytes after the SMF). */
	Bytes midi(const Record &r) const;
	static size_t smfLength(const Bytes &d, size_t off);   // 0 if no SMF at off
private:
	Bytes _d;
	std::vector<Record> _records;
};

class VocFile {
public:
	/** `directory` needs only the first 2 + 4*count bytes of the file (the file itself is ~190 MB: stream the samples from disk). */
	explicit VocFile(const Bytes &directory);
	/** Number of tracks (the file stores trackCount()+1 offsets; the last one is the file size). Track N is the recording of conversation line N. */
	size_t trackCount() const { return _offsets.size() - 1; }
	bool hasTrack(size_t n) const { return n < trackCount() && _offsets[n + 1] > _offsets[n]; }
	/** Raw 8-bit unsigned PCM (0x80 = silence), padded with 0x80 to a 4 KB boundary; the sample rate is not stored (about 22050 Hz measured). */
	uint32_t trackOffset(size_t n) const { return _offsets.at(n); }
	uint32_t trackSize(size_t n) const { return _offsets.at(n + 1) - _offsets.at(n); }
	/** Length of the samples with the trailing 0x80 padding removed. */
	static size_t trimmedSize(const uint8_t *samples, size_t size);
private:
	std::vector<uint32_t> _offsets;
};

/** RealSound PC-speaker clip (*.RS): 32-byte "STEVE" header, then `length` unsigned PCM samples whose full scale is header[6] (0x48 = PWM period in PIT ticks). */
class RsFile {
public:
	explicit RsFile(const Bytes &data);
	uint8_t kind() const { return _kind; }          // 1 or 2 (probably a repeat factor; 6 in the in-memory voice template)
	uint8_t fullScale() const { return _scale; }
	const Bytes &samples() const { return _samples; }
	/** 1193182 / fullScale() = about 16572 Hz; see analysis/xanth_overview.md (*.RS) for the open question about `kind`. */
	unsigned sampleRate() const { return 1193182 / _scale; }
	/** Unsigned 8-bit PCM scaled to the full 0..255 range. */
	Bytes toPcm8() const;
private:
	uint8_t _kind = 0, _scale = 0;
	Bytes _samples;
};

} // namespace Legend

#endif

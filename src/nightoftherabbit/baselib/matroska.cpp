#include "baselib/matroska.h"

#include <algorithm>
#include <cctype>
#include <cstring>

// The element ids (the marker bit kept).
namespace {

const uint64_t kIdEbml = 0x1A45DFA3;
const uint64_t kIdSegment = 0x18538067;
const uint64_t kIdInfo = 0x1549A966;
const uint64_t kIdTimecodeScale = 0x2AD7B1;
const uint64_t kIdDuration = 0x4489;
const uint64_t kIdTracks = 0x1654AE6B;
const uint64_t kIdTrackEntry = 0xAE;
const uint64_t kIdTrackNumber = 0xD7;
const uint64_t kIdTrackType = 0x83;
const uint64_t kIdFlagDefault = 0x88;
const uint64_t kIdDefaultDuration = 0x23E383;
const uint64_t kIdName = 0x536E;
const uint64_t kIdLanguage = 0x22B59C;
const uint64_t kIdCodecId = 0x86;
const uint64_t kIdCodecPrivate = 0x63A2;
const uint64_t kIdVideo = 0xE0;
const uint64_t kIdPixelWidth = 0xB0;
const uint64_t kIdPixelHeight = 0xBA;
const uint64_t kIdAudio = 0xE1;
const uint64_t kIdSamplingFrequency = 0xB5;
const uint64_t kIdChannels = 0x9F;
const uint64_t kIdCluster = 0x1F43B675;
const uint64_t kIdClusterTimecode = 0xE7;
const uint64_t kIdSimpleBlock = 0xA3;
const uint64_t kIdBlockGroup = 0xA0;
const uint64_t kIdBlock = 0xA1;
const uint64_t kIdBlockDuration = 0x9B;
const uint64_t kIdReferenceBlock = 0xFB;
const uint64_t kIdCues = 0x1C53BB6B;
const uint64_t kIdSeekHead = 0x114D9B74;
const uint64_t kIdTags = 0x1254C367;
const uint64_t kIdChapters = 0x1043A770;
const uint64_t kIdAttachments = 0x1941A469;

/** Whether an element of this id ends a cluster of unknown size (it is one of the elements of the segment). */
bool isSegmentChild(uint64_t id) {
	return id == kIdCluster || id == kIdCues || id == kIdSeekHead || id == kIdTags || id == kIdChapters || id == kIdAttachments ||
	       id == kIdInfo || id == kIdTracks;
}

} // End of anonymous namespace

bool TMatroskaFile::Open(const std::string &path) {
	Close();

	_file.open(path, std::ios::binary);

	if (!_file)
		return false;

	_file.seekg(0, std::ios::end);
	_fileSize = static_cast<long long>(_file.tellg());
	_file.seekg(0, std::ios::beg);

	long long pos = 0;
	uint64_t id = readId(pos);

	if (id != kIdEbml)
		return false;

	bool unknown = false;
	long long size = readSize(pos, unknown);

	if (size < 0 || unknown)
		return false;

	pos += size;
	id = readId(pos);

	if (id != kIdSegment)
		return false;

	long long segmentSize = readSize(pos, unknown);
	long long segmentEnd = unknown ? _fileSize : std::min(_fileSize, pos + segmentSize);

	while (pos < segmentEnd) {
		long long elementStart = pos;

		id = readId(pos);

		if (id == 0)
			break;

		long long elementSize = readSize(pos, unknown);

		if (elementSize < 0 && !unknown)
			break;

		long long dataEnd = unknown ? segmentEnd : std::min(segmentEnd, pos + elementSize);

		if (id == kIdInfo) {
			long long at = pos;

			while (at < dataEnd) {
				uint64_t childId = readId(at);
				bool childUnknown = false;
				long long childSize = readSize(at, childUnknown);

				if (childId == 0 || childSize < 0)
					break;

				if (childId == kIdTimecodeScale) {
					_timecodeScale = static_cast<long long>(readUnsigned(at, childSize));
				} else if (childId == kIdDuration && (childSize == 4 || childSize == 8)) {
					uint8_t raw[8];

					_file.clear();
					_file.seekg(at);
					_file.read(reinterpret_cast<char *>(raw), childSize);

					double ticks;

					if (childSize == 4) {
						uint32_t bits = (static_cast<uint32_t>(raw[0]) << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];
						float value;

						std::memcpy(&value, &bits, 4);
						ticks = value;
					} else {
						uint64_t bits = 0;

						for (int i = 0; i < 8; i++)
							bits = (bits << 8) | raw[i];

						std::memcpy(&ticks, &bits, 8);
					}

					_durationMs = static_cast<long long>(ticks * static_cast<double>(_timecodeScale) / 1000000.0);
				}

				at += childSize;
			}

			pos = dataEnd;
		} else if (id == kIdTracks) {
			parseTracks(pos, dataEnd);
			pos = dataEnd;
		} else if (id == kIdCluster) {
			pos = parseClusterEnd(pos, dataEnd, unknown);
		} else {
			pos = dataEnd;
		}

		if (pos <= elementStart)
			break;
	}

	if (_tracks.empty())
		return false;

	// (a file that does not give its length: the end of its last block)
	if (_durationMs == 0) {
		for (const Block &block : _blocks)
			_durationMs = std::max(_durationMs, block._timeMs + block._durationMs);
	}

	return true;
}

void TMatroskaFile::Close() {
	_file.close();
	_file.clear();
	_tracks.clear();
	_blocks.clear();
	_fileSize = 0;
	_timecodeScale = 1000000;
	_durationMs = 0;
}

const TMatroskaFile::Track *TMatroskaFile::FindTrack(int type) const {
	for (const Track &track : _tracks) {
		if (track._type == type)
			return &track;
	}

	return nullptr;
}

const TMatroskaFile::Track *TMatroskaFile::FindTrackByLanguage(int type, const std::string &language) const {
	std::string wanted;

	for (char c : language)
		wanted += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

	if (!wanted.empty()) {
		for (const Track &track : _tracks) {
			if (track._type == type && track._language == wanted)
				return &track;
		}
	}

	return FindTrack(type);
}

bool TMatroskaFile::ReadBlock(const Block &block, std::vector<uint8_t> &data) {
	data.resize(block._size);
	_file.clear();
	_file.seekg(block._offset);
	_file.read(reinterpret_cast<char *>(data.data()), block._size);
	return static_cast<uint32_t>(_file.gcount()) == block._size;
}

/** An element id: 1 to 4 bytes, the length from the leading zeros of the first one, the marker kept. 0 at the end of the file. */
uint64_t TMatroskaFile::readId(long long &pos) {
	if (pos >= _fileSize)
		return 0;

	uint8_t bytes[4];

	_file.clear();
	_file.seekg(pos);
	_file.read(reinterpret_cast<char *>(bytes), 1);

	int length = 1;

	while (length <= 4 && !(bytes[0] & (0x80 >> (length - 1))))
		length++;

	if (length > 4)
		return 0;

	if (length > 1)
		_file.read(reinterpret_cast<char *>(bytes + 1), length - 1);

	uint64_t id = 0;

	for (int i = 0; i < length; i++)
		id = (id << 8) | bytes[i];

	pos += length;
	return id;
}

/** The size of an element: 1 to 8 bytes with the marker taken off; all ones is an unknown size (`unknown`). -1 when it is wrong. */
long long TMatroskaFile::readSize(long long &pos, bool &unknown) {
	unknown = false;

	if (pos >= _fileSize)
		return -1;

	uint8_t bytes[8];

	_file.clear();
	_file.seekg(pos);
	_file.read(reinterpret_cast<char *>(bytes), 1);

	int length = 1;

	while (length <= 8 && !(bytes[0] & (0x80 >> (length - 1))))
		length++;

	if (length > 8)
		return -1;

	if (length > 1)
		_file.read(reinterpret_cast<char *>(bytes + 1), length - 1);

	uint64_t value = bytes[0] & ((0x80 >> (length - 1)) - 1);
	bool allOnes = (value == static_cast<uint64_t>((0x80 >> (length - 1)) - 1));

	for (int i = 1; i < length; i++) {
		value = (value << 8) | bytes[i];
		allOnes = allOnes && bytes[i] == 0xFF;
	}

	pos += length;
	unknown = allOnes;
	return static_cast<long long>(value);
}

uint64_t TMatroskaFile::readUnsigned(long long pos, long long size) {
	uint8_t bytes[8];

	if (size > 8 || size <= 0)
		return 0;

	_file.clear();
	_file.seekg(pos);
	_file.read(reinterpret_cast<char *>(bytes), size);

	uint64_t value = 0;

	for (long long i = 0; i < size; i++)
		value = (value << 8) | bytes[i];

	return value;
}

void TMatroskaFile::parseTracks(long long pos, long long end) {
	while (pos < end) {
		uint64_t id = readId(pos);
		bool unknown = false;
		long long size = readSize(pos, unknown);

		if (id == 0 || size < 0)
			return;

		if (id == kIdTrackEntry)
			parseTrackEntry(pos, std::min(end, pos + size));

		pos += size;
	}
}

void TMatroskaFile::parseTrackEntry(long long pos, long long end) {
	Track track;
	bool languageGiven = false;

	while (pos < end) {
		uint64_t id = readId(pos);
		bool unknown = false;
		long long size = readSize(pos, unknown);

		if (id == 0 || size < 0)
			break;

		auto readText = [&](long long at, long long length) {
			std::string text(static_cast<size_t>(length), '\0');

			_file.clear();
			_file.seekg(at);
			_file.read(&text[0], length);

			// (the text ends at the first zero)
			size_t zero = text.find('\0');

			if (zero != std::string::npos)
				text.resize(zero);

			return text;
		};

		auto readFloat = [&](long long at, long long length) {
			uint8_t raw[8];

			_file.clear();
			_file.seekg(at);
			_file.read(reinterpret_cast<char *>(raw), length);

			if (length == 4) {
				uint32_t bits = (static_cast<uint32_t>(raw[0]) << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];
				float value;

				std::memcpy(&value, &bits, 4);
				return static_cast<double>(value);
			}

			uint64_t bits = 0;

			for (int i = 0; i < 8; i++)
				bits = (bits << 8) | raw[i];

			double value;

			std::memcpy(&value, &bits, 8);
			return value;
		};

		switch (id) {
		case kIdTrackNumber:
			track._number = static_cast<int>(readUnsigned(pos, size));
			break;
		case kIdTrackType:
			track._type = static_cast<int>(readUnsigned(pos, size));
			break;
		case kIdFlagDefault:
			track._isDefault = readUnsigned(pos, size) != 0;
			break;
		case kIdDefaultDuration:
			track._defaultDurationNs = static_cast<long long>(readUnsigned(pos, size));
			break;
		case kIdName:
			track._name = readText(pos, size);
			break;
		case kIdLanguage:
			track._language = readText(pos, size);
			languageGiven = true;
			break;
		case kIdCodecId:
			track._codecId = readText(pos, size);
			break;
		case kIdCodecPrivate:
			track._codecPrivate.resize(static_cast<size_t>(size));
			_file.clear();
			_file.seekg(pos);
			_file.read(reinterpret_cast<char *>(track._codecPrivate.data()), size);
			break;
		case kIdVideo:
		case kIdAudio: {
			long long at = pos;
			long long childEnd = pos + size;

			while (at < childEnd) {
				uint64_t childId = readId(at);
				bool childUnknown = false;
				long long childSize = readSize(at, childUnknown);

				if (childId == 0 || childSize < 0)
					break;

				if (childId == kIdPixelWidth)
					track._width = static_cast<int>(readUnsigned(at, childSize));
				else if (childId == kIdPixelHeight)
					track._height = static_cast<int>(readUnsigned(at, childSize));
				else if (childId == kIdSamplingFrequency && (childSize == 4 || childSize == 8))
					track._sampleRate = readFloat(at, childSize);
				else if (childId == kIdChannels)
					track._channels = static_cast<int>(readUnsigned(at, childSize));

				at += childSize;
			}
			break;
		}
		default:
			break;
		}

		pos += size;
	}

	// (the language of a track that does not say it is English, as the format has it)
	if (!languageGiven)
		track._language = "eng";

	for (char &c : track._language)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

	_tracks.push_back(track);
}

/** Reads a cluster and answers where it ends (the end of the next element for a cluster of unknown size). */
long long TMatroskaFile::parseClusterEnd(long long pos, long long end, bool unknownSize) {
	long long clusterTime = 0;

	while (pos < end) {
		long long elementStart = pos;
		uint64_t id = readId(pos);

		if (id == 0)
			return end;

		if (unknownSize && isSegmentChild(id))
			return elementStart;

		bool unknown = false;
		long long size = readSize(pos, unknown);

		if (size < 0)
			return end;

		if (id == kIdClusterTimecode) {
			clusterTime = static_cast<long long>(readUnsigned(pos, size));
		} else if (id == kIdSimpleBlock) {
			addBlock(pos, size, clusterTime, 0, false, false);
		} else if (id == kIdBlockGroup) {
			long long at = pos;
			long long groupEnd = pos + size;
			long long blockPos = -1;
			long long blockSize = 0;
			long long duration = 0;
			bool referenced = false;

			while (at < groupEnd) {
				uint64_t childId = readId(at);
				bool childUnknown = false;
				long long childSize = readSize(at, childUnknown);

				if (childId == 0 || childSize < 0)
					break;

				if (childId == kIdBlock) {
					blockPos = at;
					blockSize = childSize;
				} else if (childId == kIdBlockDuration) {
					duration = static_cast<long long>(readUnsigned(at, childSize));
				} else if (childId == kIdReferenceBlock) {
					referenced = true;
				}

				at += childSize;
			}

			if (blockPos >= 0)
				addBlock(blockPos, blockSize, clusterTime, duration, true, referenced);
		}

		pos += size;
	}

	return end;
}

void TMatroskaFile::addBlock(long long pos, long long size, long long clusterTime, long long durationTicks, bool group,
                             bool referenced) {
	uint8_t header[12];
	long long toRead = std::min<long long>(sizeof(header), size);

	_file.clear();
	_file.seekg(pos);
	_file.read(reinterpret_cast<char *>(header), toRead);

	// the number of the track: 1 to 8 bytes, the marker taken off
	int length = 1;

	while (length <= 8 && !(header[0] & (0x80 >> (length - 1))))
		length++;

	if (length > 4 || size < length + 3)
		return;

	int track = header[0] & ((0x80 >> (length - 1)) - 1);

	for (int i = 1; i < length; i++)
		track = (track << 8) | header[i];

	int relative = static_cast<int16_t>((header[length] << 8) | header[length + 1]);
	uint8_t flags = header[length + 2];

	Block block;

	block._track = track;
	block._timeMs = (clusterTime + relative) * _timecodeScale / 1000000;
	block._durationMs = durationTicks * _timecodeScale / 1000000;
	block._keyFrame = group ? !referenced : (flags & 0x80) != 0;

	long long dataStart = pos + length + 3;
	long long dataSize = size - length - 3;
	int lacing = (flags >> 1) & 3;

	if (lacing == 0) {
		block._offset = dataStart;
		block._size = static_cast<uint32_t>(dataSize);
		_blocks.push_back(block);
		return;
	}

	// A laced block holds several frames (the audio tracks of the game do): the number less one, then the sizes of all but the
	// last (as sums of bytes that end at one below 255 for Xiph, a number and then differences for EBML, none for fixed sizes).
	uint8_t first;

	_file.clear();
	_file.seekg(dataStart);
	_file.read(reinterpret_cast<char *>(&first), 1);

	int frames = first + 1;
	long long at = dataStart + 1;
	std::vector<long long> sizes(static_cast<size_t>(frames), 0);
	long long total = 0;

	if (lacing == 2) {
		// fixed size
		for (int i = 0; i < frames; i++)
			sizes[static_cast<size_t>(i)] = (dataSize - 1) / frames;

		total = (dataSize - 1) / frames * frames;
		at = dataStart + 1;
	} else if (lacing == 1) {
		// Xiph
		for (int i = 0; i < frames - 1; i++) {
			uint8_t part;

			do {
				_file.clear();
				_file.seekg(at++);
				_file.read(reinterpret_cast<char *>(&part), 1);
				sizes[static_cast<size_t>(i)] += part;
			} while (part == 255);

			total += sizes[static_cast<size_t>(i)];
		}
	} else {
		// EBML: the first size is a number, the others are the differences (signed numbers) to the one before
		bool unknown;

		sizes[0] = readSize(at, unknown);
		total = sizes[0];

		for (int i = 1; i < frames - 1; i++) {
			long long start = at;
			uint8_t lead;

			_file.clear();
			_file.seekg(start);
			_file.read(reinterpret_cast<char *>(&lead), 1);

			int bytes = 1;

			while (bytes <= 8 && !(lead & (0x80 >> (bytes - 1))))
				bytes++;

			long long raw = readSize(at, unknown);
			long long bias = (1LL << (7 * bytes - 1)) - 1;

			sizes[static_cast<size_t>(i)] = sizes[static_cast<size_t>(i) - 1] + (raw - bias);
			total += sizes[static_cast<size_t>(i)];
		}
	}

	if (lacing != 2)
		sizes[static_cast<size_t>(frames) - 1] = dataStart + dataSize - at - total;

	if (sizes[static_cast<size_t>(frames) - 1] < 0)
		return;

	// (the frames have the time of the block: the file does not say more; their order is what counts)
	long long frameAt = at;

	for (int i = 0; i < frames; i++) {
		Block frame = block;

		frame._offset = frameAt;
		frame._size = static_cast<uint32_t>(sizes[static_cast<size_t>(i)]);
		frame._durationMs = durationTicks * _timecodeScale / 1000000 / frames;
		frame._timeMs = block._timeMs + frame._durationMs * i;
		_blocks.push_back(frame);
		frameAt += sizes[static_cast<size_t>(i)];
	}
}

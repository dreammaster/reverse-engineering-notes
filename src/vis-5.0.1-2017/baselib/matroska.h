// A reader of Matroska files (EBML): the tracks and the index of the blocks of a movie, and the bytes of a block on request.
// The movies of the game are Matroska files (".mkv" under another name: `video.vv000` ...) with one VP8 video, Vorbis audio
// tracks and many ASS subtitle tracks, each audio and subtitle track named by the language (ISO 639-2) it has. The original reads
// them with FFmpeg 2.8 (its ffplay); this reader is made for the one file layout the game has: the clusters are read when the file
// is opened (only the headers of the blocks, the data stays in the file) and a block is read when it is wanted. Not done: the
// lacing of blocks (the audio tracks of the game do not use it), the cues (a seek goes by the index of the blocks), the tags, the
// chapters and the attachments.
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

class TMatroskaFile {
public:
	enum TrackType {
		kVideo = 1,
		kAudio = 2,
		kSubtitle = 17
	};

	struct Track {
		int _number = 0;
		int _type = 0;
		std::string _codecId;          ///< "V_VP8", "A_VORBIS", "S_TEXT/ASS" ...
		std::string _language;         ///< "ger", "fre" ... ("eng" when the file does not say)
		std::string _name;
		bool _isDefault = true;
		std::vector<uint8_t> _codecPrivate;
		int _width = 0;                ///< of a video track
		int _height = 0;
		double _sampleRate = 0.0;      ///< of an audio track
		int _channels = 0;
		long long _defaultDurationNs = 0;
	};

	/** The header of a block (a frame of a video, a packet of audio, a line of subtitles). They are in the order of the file. */
	struct Block {
		int _track = 0;
		long long _timeMs = 0;
		long long _durationMs = 0;     ///< 0 when the file does not say
		bool _keyFrame = false;
		long long _offset = 0;         ///< of the data in the file
		uint32_t _size = 0;
	};

	TMatroskaFile() = default;
	TMatroskaFile(const TMatroskaFile &) = delete;
	TMatroskaFile &operator=(const TMatroskaFile &) = delete;

	/** Opens the file and reads the tracks and the headers of the blocks. False when it is not a Matroska file. */
	bool Open(const std::string &path);
	void Close();

	const std::vector<Track> &GetTracks() const {
		return _tracks;
	}
	const std::vector<Block> &GetBlocks() const {
		return _blocks;
	}
	/** The first track of a type (null if there is none). */
	const Track *FindTrack(int type) const;
	/** The track of a type whose language is `language` (the first one if it is empty or there is none like it). */
	const Track *FindTrackByLanguage(int type, const std::string &language) const;
	/** The length of the movie in milliseconds (the one the file says, or the end of its last block). */
	long long GetDurationMs() const {
		return _durationMs;
	}
	/** Reads the data of a block. */
	bool ReadBlock(const Block &block, std::vector<uint8_t> &data);

private:
	uint64_t readId(long long &pos);
	long long readSize(long long &pos, bool &unknown);
	void parseTracks(long long pos, long long end);
	void parseTrackEntry(long long pos, long long end);
	long long parseClusterEnd(long long pos, long long end, bool unknownSize);
	void addBlock(long long pos, long long size, long long clusterTime, long long durationTicks, bool group, bool referenced);
	uint64_t readUnsigned(long long pos, long long size);

	std::ifstream _file;
	long long _fileSize = 0;
	long long _timecodeScale = 1000000;
	long long _durationMs = 0;
	std::vector<Track> _tracks;
	std::vector<Block> _blocks;
};

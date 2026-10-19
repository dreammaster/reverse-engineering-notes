#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "baselib/matroska.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

typedef std::vector<unsigned char> Bytes;

// EBML builders: an element is its id, its size (8 bytes with the marker, so a size never has to be worked out) and the data.
static void put(Bytes &out, const Bytes &more) {
	out.insert(out.end(), more.begin(), more.end());
}

static Bytes idBytes(unsigned long long id) {
	Bytes result;

	for (int shift = 24; shift >= 0; shift -= 8) {
		if ((id >> shift) != 0 || !result.empty())
			result.push_back(static_cast<unsigned char>(id >> shift));
	}
	return result;
}

static Bytes element(unsigned long long id, const Bytes &data) {
	Bytes out = idBytes(id);
	unsigned long long size = data.size();

	out.push_back(0x01);
	for (int shift = 48; shift >= 0; shift -= 8)
		out.push_back(static_cast<unsigned char>(size >> shift));
	put(out, data);
	return out;
}

static Bytes number(unsigned long long value, int length = 4) {
	Bytes out;

	for (int i = length - 1; i >= 0; i--)
		out.push_back(static_cast<unsigned char>(value >> (8 * i)));
	return out;
}

static Bytes text(const char *value) {
	return Bytes(value, value + std::string(value).size());
}

static Bytes block(int track, int relative, int flags, const char *payload) {
	Bytes out;

	out.push_back(static_cast<unsigned char>(0x80 | track));
	out.push_back(static_cast<unsigned char>(relative >> 8));
	out.push_back(static_cast<unsigned char>(relative));
	out.push_back(static_cast<unsigned char>(flags));
	put(out, text(payload));
	return out;
}

int main() {
	// a made-up file: a video track, a Vorbis track without a language and a subtitle track in German
	Bytes tracks;

	put(tracks, element(0xAE, [] {
		Bytes entry;

		put(entry, element(0xD7, number(1, 1)));
		put(entry, element(0x83, number(1, 1)));
		put(entry, element(0x86, text("V_VP8")));
		put(entry, element(0x23E383, number(40000000, 4)));
		put(entry, element(0xE0, [] {
			Bytes video;

			put(video, element(0xB0, number(1920, 2)));
			put(video, element(0xBA, number(1080, 2)));
			return video;
		}()));
		return entry;
	}()));
	put(tracks, element(0xAE, [] {
		Bytes entry;

		put(entry, element(0xD7, number(2, 1)));
		put(entry, element(0x83, number(2, 1)));
		put(entry, element(0x86, text("A_VORBIS")));
		put(entry, element(0xE1, [] {
			Bytes audio;
			unsigned long long bits = 0x40E5888000000000ull;  // 44100.0

			put(audio, element(0xB5, number(bits, 8)));
			put(audio, element(0x9F, number(2, 1)));
			return audio;
		}()));
		return entry;
	}()));
	put(tracks, element(0xAE, [] {
		Bytes entry;

		put(entry, element(0xD7, number(3, 1)));
		put(entry, element(0x83, number(17, 1)));
		put(entry, element(0x86, text("S_TEXT/ASS")));
		put(entry, element(0x22B59C, text("GER")));
		return entry;
	}()));

	Bytes info;

	put(info, element(0x2AD7B1, number(1000000, 4)));
	put(info, element(0x4489, number(0x40A3880000000000ull, 8)));  // 2500.0 ms

	Bytes cluster;

	put(cluster, element(0xE7, number(1000, 2)));
	put(cluster, element(0xA3, block(1, 0, 0x80, "key")));
	put(cluster, element(0xA3, block(1, 40, 0x00, "delta")));
	put(cluster, element(0xA0, [] {
		Bytes group;

		put(group, element(0xA1, block(3, 100, 0, "text")));
		put(group, element(0x9B, number(1500, 2)));
		return group;
	}()));

	Bytes segmentBody;

	put(segmentBody, element(0x1549A966, info));
	put(segmentBody, element(0x1654AE6B, tracks));
	put(segmentBody, element(0x1F43B675, cluster));

	Bytes file;

	put(file, element(0x1A45DFA3, element(0x4282, text("matroska"))));
	put(file, element(0x18538067, segmentBody));

	{
		std::ofstream out("matroska_made.mkv", std::ios::binary);

		out.write(reinterpret_cast<const char *>(file.data()), file.size());
	}

	TMatroskaFile mkv;

	CHECK(mkv.Open("matroska_made.mkv"));
	CHECK(mkv.GetTracks().size() == 3);
	CHECK(mkv.GetDurationMs() == 2500);

	const TMatroskaFile::Track *video = mkv.FindTrack(TMatroskaFile::kVideo);

	CHECK(video && video->_codecId == "V_VP8" && video->_width == 1920 && video->_height == 1080);
	CHECK(video && video->_defaultDurationNs == 40000000 && video->_language == "eng");

	const TMatroskaFile::Track *audio = mkv.FindTrack(TMatroskaFile::kAudio);

	CHECK(audio && audio->_sampleRate == 44100.0 && audio->_channels == 2);

	const TMatroskaFile::Track *subtitle = mkv.FindTrackByLanguage(TMatroskaFile::kSubtitle, "ger");

	CHECK(subtitle && subtitle->_number == 3 && subtitle->_language == "ger");
	CHECK(mkv.FindTrackByLanguage(TMatroskaFile::kSubtitle, "fre") == subtitle);   // a language that is not there: the first track

	CHECK(mkv.GetBlocks().size() == 3);
	if (mkv.GetBlocks().size() == 3) {
		const TMatroskaFile::Block &first = mkv.GetBlocks()[0];
		const TMatroskaFile::Block &second = mkv.GetBlocks()[1];
		const TMatroskaFile::Block &third = mkv.GetBlocks()[2];

		CHECK(first._track == 1 && first._timeMs == 1000 && first._keyFrame && first._size == 3);
		CHECK(second._track == 1 && second._timeMs == 1040 && !second._keyFrame && second._size == 5);
		CHECK(third._track == 3 && third._timeMs == 1100 && third._durationMs == 1500 && third._keyFrame);

		std::vector<unsigned char> data;

		CHECK(mkv.ReadBlock(second, data));
		CHECK(std::string(data.begin(), data.end()) == "delta");
		CHECK(mkv.ReadBlock(third, data));
		CHECK(std::string(data.begin(), data.end()) == "text");
	}

	CHECK(!mkv.Open("matroska_not_there.mkv"));
	{
		std::ofstream out("matroska_not.mkv", std::ios::binary);

		out << "this is not a matroska file";
	}
	CHECK(!mkv.Open("matroska_not.mkv"));
	mkv.Close();
	std::remove("matroska_made.mkv");
	std::remove("matroska_not.mkv");

	// a movie of the real data, when the test is run from the game directory
	if (mkv.Open("videos/video.vv001")) {
		const TMatroskaFile::Track *realVideo = mkv.FindTrack(TMatroskaFile::kVideo);

		CHECK(realVideo && realVideo->_codecId == "V_VP8");
		printf("real movie: %lld ms, %d tracks, %d blocks\n", mkv.GetDurationMs(), (int)mkv.GetTracks().size(), (int)mkv.GetBlocks().size());
		CHECK(mkv.GetDurationMs() > 1000 && mkv.GetBlocks().size() > 20);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

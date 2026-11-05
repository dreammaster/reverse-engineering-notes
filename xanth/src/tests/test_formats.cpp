// Reads every data file of the game with the readers of legend/ and checks the structural invariants that were established while reverse engineering the formats
// (decoded size == width * height, region ids, one MIDI file per tune, ...). Needs the game folder: XANTH_DATA=<folder>; without it the test is skipped (exit 0).
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "legend/audiofiles.h"
#include "legend/messages.h"
#include "legend/pic.h"
#include "legend/rgn.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, ...)                                                      \
	do {                                                                      \
		g_checks++;                                                           \
		if (!(cond)) {                                                        \
			g_failures++;                                                     \
			printf("FAIL %s:%d: ", __FILE__, __LINE__);                       \
			printf(__VA_ARGS__);                                              \
			printf("\n");                                                     \
		}                                                                     \
	} while (0)

static std::string numbered(const char *prefix, int n, const char *ext) {
	char b[64];
	snprintf(b, sizeof(b), "%s%02d.%s", prefix, n, ext);
	return b;
}

int main() {
	const char *dir = getenv("XANTH_DATA");
	if (!dir) {
		printf("test_formats: XANTH_DATA not set, skipped\n");
		return 0;
	}
	Common::GameFiles files;
	if (!files.setFolder(dir)) {
		printf("test_formats: cannot read %s\n", dir);
		return 1;
	}

	// pictures: every entry of every XANTH_nn.PIC decodes to width * height pixels; regions (.RGN) belong to the pictures of the same number
	size_t pictures = 0, regionScenes = 0, regions = 0;
	for (int n = 0; n < 100; n++) {
		std::string pic = numbered("XANTH_", n, "PIC");
		if (!files.exists(pic))
			continue;
		Common::Bytes data;
		CHECK(files.load(pic, data), "load %s", pic.c_str());
		Legend::PicFile pf;
		CHECK(pf.load(data), "directory of %s", pic.c_str());
		for (size_t i = 0; i < pf.size(); i++) {
			if (pf.entry(i).offset == 0)
				continue;
			Legend::PicImage im;
			CHECK(pf.decode(i, im), "%s entry %u", pic.c_str(), (unsigned)i);
			pictures++;
		}
		std::string rgn = numbered("XANTH_", n, "RGN");
		if (files.exists(rgn)) {
			Common::Bytes rd;
			Legend::RgnFile rf;
			CHECK(files.load(rgn, rd) && rf.load(rd), "regions %s", rgn.c_str());
			for (size_t i = 0; i < rf.size(); i++) {
				if (rf.scene(i).regions.empty())
					continue;                                     // the directory has unused slots
				CHECK(i < pf.size() && pf.entry(i).offset != 0, "%s scene %u has regions but %s has no such picture", rgn.c_str(), (unsigned)i, pic.c_str());
				regionScenes++;
				regions += rf.scene(i).regions.size();
			}
		}
	}
	printf("pictures decoded: %u; scenes with regions: %u (%u regions)\n", (unsigned)pictures, (unsigned)regionScenes, (unsigned)regions);
	CHECK(pictures == 3294, "expected 3,294 pictures, found %u", (unsigned)pictures);

	// text
	Common::Bytes sd;
	Legend::MessageFile mf;
	CHECK(files.load("XANTHSTR.DAT", sd) && mf.load(sd), "XANTHSTR.DAT");
	size_t strings = 0;
	for (size_t b = 0; b < mf.blockCount(); b++)
		strings += mf.stringCount(b);
	printf("text blocks: %u, strings: %u\n", (unsigned)mf.blockCount(), (unsigned)strings);
	CHECK(mf.blockCount() == 80 && strings == 5542, "expected 80 blocks / 5,542 strings");

	// music
	size_t tunes = 0;
	for (int n = 1; n < 100; n++) {
		std::string mus = numbered("XANTH_", n, "MUS");
		if (!files.exists(mus))
			continue;
		Common::Bytes md;
		Legend::MusFile mu;
		CHECK(files.load(mus, md) && mu.load(md), "%s", mus.c_str());
		for (size_t i = 0; i < mu.records().size(); i++) {
			const Legend::MusFile::Record &r = mu.records()[i];
			if (r.offset == 0 || r.offset >= md.size())
				continue;
			Common::Bytes midi;
			if (mu.midi(r, midi))
				tunes++;
		}
	}
	printf("MIDI files: %u\n", (unsigned)tunes);
	CHECK(tunes == 86, "expected 86 MIDI files, found %u", (unsigned)tunes);

	// voice directory (CD version only): its last offset is the file size
	if (files.exists("XANTH_01.VOC")) {
		Common::Bytes head;
		CHECK(files.loadPart("XANTH_01.VOC", 0, 2, head), "VOC head");
		size_t count = head.size() == 2 ? (size_t)(head[0] | (head[1] << 8)) : 0;
		Common::Bytes dir;
		Legend::VocFile vf;
		CHECK(files.loadPart("XANTH_01.VOC", 0, 2 + 4 * count, dir) && vf.load(dir), "VOC directory");
		CHECK(vf.trackCount() == 1326, "expected 1,326 voice tracks, found %u", (unsigned)vf.trackCount());
		CHECK(vf.trackCount() && vf.trackOffset(vf.trackCount()) == files.fileSize("XANTH_01.VOC"), "last VOC offset is the file size");
	} else {
		printf("no XANTH_01.VOC: voice disabled (floppy version)\n");
	}

	// sound effects
	static const char *const kClips[] = {"APPLOPEN", "BARK", "BEAST", "BEE", "BUBBLE", "CREAK", "DIAL", "DOORBELL", "ENGINE", "FLY", "FRIDGE", "FRNTDOOR", "LAUGH", "MAIL", "MAILCLS",
	                                     "OIL", "PCON", "PEND", "PHONE", "PORTCLLS", "SQUISH", "STEPS", "THUNDER", "WHEE", "WRITE"};
	size_t clips = 0;
	for (int i = 0; i < ARRAYSIZE(kClips); i++) {
		std::string name = std::string(kClips[i]) + ".RS";
		if (!files.exists(name))
			continue;
		Common::Bytes cd;
		Legend::RsFile rs;
		CHECK(files.load(name, cd) && rs.load(cd), "%s", name.c_str());
		clips++;
	}
	printf("RealSound clips: %u\n", (unsigned)clips);
	CHECK(clips > 20, "expected the RealSound clips");

	printf("test_formats: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}

// Command line front end used to verify the C++ readers against the Python references on real data.
//   shan_tool shanstr FILE [dump OUT]     decode every string ("block:index<TAB>text", same escaping as shanstr.py)
//   shan_tool pic FILE                    directory + per-entry checksums
//   shan_tool rgn FILE                    regions per directory entry
//   shan_tool fnt FILE                    header summary and a glyph checksum
//   shan_tool q FILE                      per-frame checksums of the decoded 640-pitch image and palette
#include "fnt.h"
#include "mus.h"
#include "objectdat.h"
#include "pic.h"
#include "qvideo.h"
#include "rgn.h"
#include "shanstr.h"

#include <cstdio>
#include <cstring>

using namespace shannara;

static uint64_t fnv(const uint8_t *p, size_t n, uint64_t h = 1469598103934665603ull) {
	for (size_t i = 0; i < n; i++) {
		h ^= p[i];
		h *= 1099511628211ull;
	}
	return h;
}

static int cmdShanStr(int argc, char **argv) {
	ShanStr s(readFile(argv[2]));
	int total = 0;
	for (int b = 0; b < s.blockCount(); b++)
		total += s.stringCount(b);
	printf("blocks: %d strings: %d phrases: %zu computed end: %zu file size: %zu\n", s.blockCount(), total, s.phraseCount(), s.computedEnd(), s.fileSize());
	if (argc >= 5 && !strcmp(argv[3], "dump")) {
		FILE *f = fopen(argv[4], "wb");
		if (!f)
			return 1;
		for (int b = 0; b < s.blockCount(); b++)
			for (int i = 0; i < s.stringCount(b); i++) {
				std::string t = s.string(b, i), esc;
				for (unsigned char c : t) {
					if (c == '\n') esc += "\\n";
					else if (c == '\r') esc += "\\r";
					else if (c == 0x0B) esc += "\\v";
					else esc.push_back((char)c);
				}
				fprintf(f, "%d:%d\t", b, i);
			fwrite(esc.data(), 1, esc.size(), f); // strings keep their trailing NUL symbol, as in the Python reference
			fputc('\n', f);
			}
		fclose(f);
	}
	return 0;
}

static int cmdPic(char **argv) {
	PicFile p(readFile(argv[2]));
	printf("entries: %zu\n", p.entries().size());
	for (const PicEntry &e : p.entries()) {
		PicImage img = p.load(e.index);
		uint64_t h = fnv(img.pixels.data(), img.pixels.size());
		if (img.hasPalette)
			h = fnv(img.palette, 768, h);
		for (auto &f : img.offsets) {
			uint8_t t[4] = {(uint8_t)f.x, (uint8_t)(f.x >> 8), (uint8_t)f.y, (uint8_t)(f.y >> 8)};
			h = fnv(t, 4, h);
		}
		printf("%d off=%u frames=%u flags=%02x w=%u h=%u pal=%d px=%zu hash=%016llx\n", e.index, e.offset, e.frames, e.flags, e.w, e.h,
		       img.hasPalette, img.pixels.size(), (unsigned long long)h);
	}
	return 0;
}

static int cmdRgn(char **argv) {
	RgnFile r(readFile(argv[2]));
	size_t withRegions = 0;
	for (const RgnEntry &e : r.entries())
		withRegions += e.offset != 0;
	printf("%zu entries, %zu with regions\n", r.entries().size(), withRegions);
	for (size_t i = 0; i < r.entries().size(); i++) {
		const RgnEntry &e = r.entries()[i];
		if (!e.offset)
			continue;
		uint64_t h = 1469598103934665603ull;
		for (const Region &g : e.regions) {
			uint8_t id[3] = {(uint8_t)g.id, (uint8_t)(g.id >> 8), (uint8_t)g.rect};
			h = fnv(id, 3, h);
			for (int16_t v : g.pts) {
				uint8_t t[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
				h = fnv(t, 2, h);
			}
		}
		printf("%zu base=%u count=%u hash=%016llx\n", i, e.baseId, e.count, (unsigned long long)h);
	}
	return 0;
}

static int cmdFnt(char **argv) {
	Bytes d = readFile(argv[2]);
	Font f(d);
	uint64_t h = 1469598103934665603ull;
	for (int c = f.firstChar(); c <= f.lastChar(); c++)
		h = fnv(f.glyph(c), (size_t)f.bytesPerRow() * f.height(), h);
	printf("first %d last %d bpr %d height %d bpp %d size %zu expected %zu glyphhash=%016llx\n", f.firstChar(), f.lastChar(), f.bytesPerRow(),
	       f.height(), f.bitsPerPixel(), f.actualSize(), f.expectedSize(), (unsigned long long)h);
	return 0;
}

static int cmdQ(char **argv) {
	QVideo q(readFile(argv[2]));
	printf("blocks %d x %d fps %d header frames %d\n", q.blocksWide(), q.blocksHigh(), q.fps(), q.headerFrameCount());
	QFrame fr;
	int n = 0, bad = 0;
	while (q.nextFrame(fr)) {
		uint64_t h = 1469598103934665603ull;
		for (int y = 0; y < q.height(); y++)
			h = fnv(&q.video()[(size_t)y * 640], (size_t)q.width(), h);
		h = fnv(q.palette(), 768, h);
		printf("frame %d type %d stream %zu consumed %zu hash=%016llx\n", fr.index, fr.type, fr.streamBytes, fr.consumed, (unsigned long long)h);
		bad += fr.streamBytes != fr.consumed;
		n++;
	}
	printf("decoded frames: %d mismatched: %d\n", n, bad);
	return 0;
}

static int cmdMus(char **argv) {
	MusFile m(readFile(argv[2]));
	int valid = 0;
	for (int i = 0; i < m.trackCount(); i++)
		valid += m.track(i).validXmidi;
	printf("tracks %d valid xmidi %d\n", m.trackCount(), valid);
	for (int i = 0; i < m.trackCount(); i++)
		printf("%d off=%u size=%u hash=%016llx\n", i, m.track(i).offset, m.track(i).size, (unsigned long long)fnv(m.data(i), m.track(i).size));
	return 0;
}

static int cmdObj(char **argv) {
	ObjectDat o(readFile(argv[2]));
	printf("pool %zu strings %d\n", o.poolSize(), o.stringCount());
	uint32_t off = 1;
	for (int i = 0; i < 5; i++) {
		std::string t = o.text(off);
		printf("%u %s\n", off, t.c_str());
		off += (uint32_t)t.size() + 1;
	}
	return 0;
}

// Differential fuzz vectors written by tests/fuzz_q.py: u32 W,H,minCol,minRow,maxCol,maxRow, u8 raw+3 pad, qmap[256],
// video0 (H*0xA00 + 4*640 bytes), comp (same size), u32 streamLen, stream.
static int cmdQVec(char **argv) {
	Bytes d = readFile(argv[2]);
	Cursor c(d);
	int n = 0;
	while (c.left() > 0) {
		int W = c.u32(), H = c.u32(), minCol = c.u32(), minRow = c.u32(), maxCol = c.u32(), maxRow = c.u32();
		bool raw = c.u8() != 0;
		c.skip(3);
		uint8_t qmap[256];
		for (int i = 0; i < 256; i++)
			qmap[i] = c.u8();
		size_t vsz = (size_t)H * QBlockDecoder::kRow + 4 * QBlockDecoder::kPitch;
		Bytes video(0x200000, 0), comp(vsz + 0x10000, 0);
		for (size_t i = 0; i < vsz; i++)
			video[i] = c.u8();
		for (size_t i = 0; i < vsz; i++)
			comp[i] = c.u8();
		size_t len = c.u32();
		const uint8_t *stream = c.ptr();
		c.skip(len);
		QBlockDecoder dec(W, H, qmap, video, &comp, raw);
		size_t used = dec.decode(stream, len, minCol, minRow, maxCol, maxRow);
		printf("%d consumed %zu hash=%016llx\n", n++, used, (unsigned long long)fnv(video.data(), (size_t)H * QBlockDecoder::kRow));
	}
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: shan_tool shanstr|pic|rgn|fnt|q FILE [...]\n");
		return 2;
	}
	try {
		if (!strcmp(argv[1], "shanstr")) return cmdShanStr(argc, argv);
		if (!strcmp(argv[1], "pic")) return cmdPic(argv);
		if (!strcmp(argv[1], "rgn")) return cmdRgn(argv);
		if (!strcmp(argv[1], "fnt")) return cmdFnt(argv);
		if (!strcmp(argv[1], "q")) return cmdQ(argv);
		if (!strcmp(argv[1], "qvec")) return cmdQVec(argv);
		if (!strcmp(argv[1], "mus")) return cmdMus(argv);
		if (!strcmp(argv[1], "objdat")) return cmdObj(argv);
	} catch (const std::exception &e) {
		fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
	fprintf(stderr, "unknown command\n");
	return 2;
}

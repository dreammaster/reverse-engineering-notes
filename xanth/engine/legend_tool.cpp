/* Command line front end for legend_formats: inspects and converts Companions of Xanth data and self-checks every file in a game folder.
 *
 *   legend_tool check <gamedir>                    decode everything and report mismatches (exit status 1 on any failure)
 *   legend_tool pic <XANTH_nn.PIC> [entry [out.ppm]]
 *   legend_tool str <XANTHSTR.DAT> [block index | dump]
 *   legend_tool rgn <XANTH_nn.RGN> [entry]
 *   legend_tool mus <XANTH_nn.MUS> [tune arrangement out.mid]
 *   legend_tool voc <XANTH_01.VOC> [track out.wav]
 */
#include "legend_formats.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <strings.h>

using namespace Legend;

static std::vector<std::string> listDir(const std::string &dir, const char *ext) {
	std::vector<std::string> out;
	DIR *d = opendir(dir.c_str());
	if (!d)
		return out;
	while (dirent *e = readdir(d)) {
		std::string n = e->d_name;
		if (n.size() > 4 && strcasecmp(n.c_str() + n.size() - 4, ext) == 0)
			out.push_back(n);
	}
	closedir(d);
	std::sort(out.begin(), out.end());
	return out;
}

static void writePpm(const char *path, const PicFile::Image &im) {
	FILE *f = fopen(path, "wb");
	if (!f)
		throw FormatError(std::string("cannot write ") + path);
	fprintf(f, "P6\n%u %u\n255\n", im.width, im.height);
	for (uint8_t px : im.pixels) {
		uint8_t rgb[3] = {palette8(im.palette[px * 3]), palette8(im.palette[px * 3 + 1]), palette8(im.palette[px * 3 + 2])};
		fwrite(rgb, 1, 3, f);
	}
	fclose(f);
}

static void writeWav(const char *path, const uint8_t *pcm, size_t n, unsigned rate) {
	FILE *f = fopen(path, "wb");
	if (!f)
		throw FormatError(std::string("cannot write ") + path);
	auto w32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
	auto w16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
	fwrite("RIFF", 1, 4, f);
	w32((uint32_t)(36 + n));
	fwrite("WAVEfmt ", 1, 8, f);
	w32(16);
	w16(1);
	w16(1);
	w32(rate);
	w32(rate);
	w16(1);
	w16(8);
	fwrite("data", 1, 4, f);
	w32((uint32_t)n);
	fwrite(pcm, 1, n, f);
	fclose(f);
}

static int check(const std::string &dir) {
	int bad = 0;
	auto fail = [&](const std::string &what) { printf("FAIL %s\n", what.c_str()); bad++; };
	for (auto &n : listDir(dir, ".PIC")) {
		try {
			PicFile pic(readFile(dir + "/" + n));
			size_t ok = 0;
			for (size_t i = 0; i < pic.size(); i++) {
				try {
					pic.decode(i);
					ok++;
				} catch (const std::exception &e) {
					fail(n + " entry " + std::to_string(i) + ": " + e.what());
				}
			}
			printf("%s: %zu/%zu pictures decode\n", n.c_str(), ok, pic.size());
		} catch (const std::exception &e) {
			fail(n + ": " + e.what());
		}
	}
	for (auto &n : listDir(dir, ".RGN")) {
		try {
			RgnFile r(readFile(dir + "/" + n));
			size_t with = 0, regs = 0;
			for (size_t i = 0; i < r.size(); i++) {
				with += !r.scene(i).regions.empty();
				regs += r.scene(i).regions.size();
			}
			printf("%s: %zu entries, %zu with regions, %zu regions\n", n.c_str(), r.size(), with, regs);
		} catch (const std::exception &e) {
			fail(n + ": " + e.what());
		}
	}
	for (auto &n : listDir(dir, ".DAT")) {
		if (strncasecmp(n.c_str(), "XANTHSTR", 8) != 0)
			continue;
		try {
			MessageFile m(readFile(dir + "/" + n));
			size_t strs = 0, empty = 0;
			for (size_t b = 0; b < m.blockCount(); b++)
				for (size_t i = 0; i < m.stringCount(b); i++, strs++)
					empty += m.string(b, i).empty();
			printf("%s: %zu blocks, %zu strings (%zu empty)\n", n.c_str(), m.blockCount(), strs, empty);
		} catch (const std::exception &e) {
			fail(n + ": " + e.what());
		}
	}
	for (auto &n : listDir(dir, ".MUS")) {
		try {
			Bytes d = readFile(dir + "/" + n);
			MusFile m(d);
			size_t used = 0, good = 0;
			for (size_t i = 0; i < m.records().size(); i++) {
				const MusFile::Record &r = m.records()[i];
				if (r.offset == 0 || r.offset >= d.size())
					continue;
				used++;
				size_t len = MusFile::smfLength(d, r.offset);
				good += len && len + (i & 1 ? 4 : 0) == r.size;
			}
			printf("%s: %zu records used, %zu consistent\n", n.c_str(), used, good);
			if (used != good)
				fail(n + ": inconsistent MIDI sizes");
		} catch (const std::exception &e) {
			fail(n + ": " + e.what());
		}
	}
	for (auto &n : listDir(dir, ".VOC")) {
		try {
			FILE *f = fopen((dir + "/" + n).c_str(), "rb");
			if (!f)
				throw FormatError("cannot open");
			uint8_t hdr[2];
			if (fread(hdr, 1, 2, f) != 2)
				throw FormatError("short file");
			Bytes dirBytes(2 + 4 * (hdr[0] | (hdr[1] << 8)));
			rewind(f);
			if (fread(dirBytes.data(), 1, dirBytes.size(), f) != dirBytes.size())
				throw FormatError("short directory");
			fseek(f, 0, SEEK_END);
			long sz = ftell(f);
			fclose(f);
			VocFile v(dirBytes);
			size_t voiced = 0;
			bool mono = true;
			for (size_t t = 0; t < v.trackCount(); t++) {
				voiced += v.hasTrack(t);
				mono &= v.trackOffset(t + 1) >= v.trackOffset(t);
			}
			printf("%s: %zu tracks, %zu voiced, last offset %u, file size %ld\n", n.c_str(), v.trackCount(), voiced, v.trackOffset(v.trackCount()), sz);
			if (!mono || v.trackOffset(v.trackCount()) != (uint32_t)sz)
				fail(n + ": directory inconsistent");
		} catch (const std::exception &e) {
			fail(n + ": " + e.what());
		}
	}
	printf(bad ? "%d failure(s)\n" : "all checks passed\n", bad);
	return bad ? 1 : 0;
}

int main(int argc, char **argv) {
	try {
		if (argc < 3) {
			fprintf(stderr, "usage: legend_tool check <gamedir> | pic|str|rgn|mus|voc <file> [args]\n");
			return 2;
		}
		std::string cmd = argv[1], file = argv[2];
		if (cmd == "check")
			return check(file);
		if (cmd == "pic") {
			PicFile pic(readFile(file));
			if (argc < 4) {
				for (size_t i = 0; i < pic.size(); i++) {
					const PicFile::Entry &e = pic.entry(i);
					printf("%zu: %ux%u flags %02x offsets %u at %u\n", i, e.width, e.height, e.flags, e.count, e.offset);
				}
				return 0;
			}
			PicFile::Image im = pic.decode(atoi(argv[3]));
			printf("%ux%u at (%d,%d) %s\n", im.width, im.height, im.x, im.y, im.hasPalette ? "with palette" : "no palette");
			if (argc > 4 && im.hasPalette)
				writePpm(argv[4], im);
		} else if (cmd == "str") {
			MessageFile m(readFile(file));
			if (argc >= 5) {
				printf("%s\n", m.string(atoi(argv[3]), atoi(argv[4])).c_str());
			} else {
				for (size_t b = 0; b < m.blockCount(); b++)
					for (size_t i = 0; i < m.stringCount(b); i++)
						printf("%zu:%zu\t%s\n", b, i, m.string(b, i).c_str());
			}
		} else if (cmd == "rgn") {
			RgnFile r(readFile(file));
			for (size_t i = 0; i < r.size(); i++) {
				if (argc > 3 && (size_t)atoi(argv[3]) != i)
					continue;
				const RgnFile::Scene &s = r.scene(i);
				if (s.regions.empty())
					continue;
				printf("entry %zu base %u:", i, s.baseId);
				for (auto &g : s.regions)
					printf(" %d%s", g.id, g.rect ? "r" : "p");
				printf("\n");
			}
		} else if (cmd == "mus") {
			MusFile m(readFile(file));
			if (argc < 6) {
				for (size_t i = 0; i < m.records().size(); i++)
					if (m.records()[i].offset)
						printf("%zu: offset %u size %u (%u,%u,%u)\n", i, m.records()[i].offset, m.records()[i].size, m.records()[i].a, m.records()[i].b, m.records()[i].c);
				return 0;
			}
			const MusFile::Record *r = m.tune(atoi(argv[3]), atoi(argv[4]));
			if (!r)
				throw FormatError("no such tune");
			Bytes mid = m.midi(*r);
			FILE *f = fopen(argv[5], "wb");
			fwrite(mid.data(), 1, mid.size(), f);
			fclose(f);
		} else if (cmd == "voc") {
			FILE *f = fopen(file.c_str(), "rb");
			if (!f)
				throw FormatError("cannot open " + file);
			uint8_t hdr[2];
			if (fread(hdr, 1, 2, f) != 2)
				throw FormatError("short file");
			Bytes dirBytes(2 + 4 * (hdr[0] | (hdr[1] << 8)));
			rewind(f);
			if (fread(dirBytes.data(), 1, dirBytes.size(), f) != dirBytes.size())
				throw FormatError("short directory");
			VocFile v(dirBytes);
			if (argc < 5) {
				printf("%zu tracks\n", v.trackCount());
				return 0;
			}
			size_t t = atoi(argv[3]);
			Bytes pcm(v.trackSize(t));
			fseek(f, (long)v.trackOffset(t), SEEK_SET);
			if (fread(pcm.data(), 1, pcm.size(), f) != pcm.size())
				throw FormatError("short track");
			fclose(f);
			writeWav(argv[4], pcm.data(), VocFile::trimmedSize(pcm.data(), pcm.size()), 22050);
		} else {
			fprintf(stderr, "unknown command\n");
			return 2;
		}
	} catch (const std::exception &e) {
		fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
	return 0;
}

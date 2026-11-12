// Fonts and surfaces: all six XANTH_nn.FNT files parse, text measures and draws; a picture blits to a surface and converts to RGB.
// With XANTH_SHOTS=<folder> a PPM of a sample line of every font and of the first picture is written for a look.
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "legend/font.h"
#include "legend/pic.h"
#include "legend/surface.h"
#include "xanth/mainscreen.h"

static int g_failures = 0, g_checks = 0;

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


// Compares our screen with an oracle screenshot (PPM written by the host's HOST_SHOT): mismatching pixels per region, and a PPM that shows them in red.
static bool loadPpm(const char *path, std::vector<byte> &rgb) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	int w = 0, h = 0, m = 0;
	bool ok = fscanf(f, "P6 %d %d %d", &w, &h, &m) == 3 && w == 320 && h == 200 && fgetc(f) >= 0;
	rgb.resize(320 * 200 * 3);
	ok = ok && fread(&rgb[0], 1, rgb.size(), f) == rgb.size();
	fclose(f);
	return ok;
}

static void compareScreens(const Legend::Surface &ours, const std::vector<byte> &oracle, const char *diffPath) {
	static const struct { const char *name; int x1, y1, x2, y2; } kRegions[] = {
		{"verbs", 0, 0, 49, 127}, {"picture window", 50, 3, 315, 126}, {"text window", 50, 127, 315, 150}, {"compass", 0, 127, 49, 166}, {"buttons", 0, 167, 49, 199},
		{"slot panel", 50, 151, 84, 199}, {"list panel", 85, 151, 289, 199}, {"scroll panel", 290, 151, 319, 199}, {"everything", 0, 0, 319, 199}};
	std::vector<byte> rgb(320 * 200 * 3);
	ours.toRgb(&rgb[0]);
	std::vector<byte> diff(rgb);
	for (int i = 0; i < 320 * 200; i++) {
		if (memcmp(&rgb[i * 3], &oracle[i * 3], 3) != 0) {
			diff[i * 3] = 255;
			diff[i * 3 + 1] = diff[i * 3 + 2] = 0;
		}
	}
	int hist[256] = {0};
	for (int i = 0; i < 320 * 200; i++)
		if (diff[i * 3] == 255 && diff[i * 3 + 1] == 0 && memcmp(&rgb[i * 3], &oracle[i * 3], 3) != 0)
			hist[ours.pixels()[i]]++;
	printf("  palette indices of differing pixels:");
	for (int i = 0; i < 256; i++)
		if (hist[i] > 20)
			printf(" %d:%d", i, hist[i]);
	printf("\n");
	for (int r = 0; r < ARRAYSIZE(kRegions); r++) {
		int bad = 0, total = 0;
		for (int y = kRegions[r].y1; y <= kRegions[r].y2; y++)
			for (int x = kRegions[r].x1; x <= kRegions[r].x2; x++, total++)
				bad += memcmp(&rgb[(y * 320 + x) * 3], &oracle[(y * 320 + x) * 3], 3) != 0;
		printf("  %-15s %6d of %6d pixels differ\n", kRegions[r].name, bad, total);
	}
	if (diffPath) {
		FILE *f = fopen(diffPath, "wb");
		if (f) {
			fprintf(f, "P6\n320 200\n255\n");
			fwrite(&diff[0], 1, diff.size(), f);
			fclose(f);
		}
	}
}

int main() {
	// a hand-made font: 2 glyphs, 8 wide, 2 tall
	{
		Common::Bytes d(10, 0);
		d[1] = 1; d[2] = 1; d[3] = 'A'; d[4] = 'B'; d[5] = 3; d[6] = 2; d[7] = 8; d[8] = 1;
		const byte glyphs[4] = {0xFF, 0x81, 0x80, 0x01};
		d.insert(d.end(), glyphs, glyphs + 4);
		Legend::Font f;
		CHECK(f.load(d), "synthetic font loads");
		Legend::Surface s(16, 4);
		CHECK(f.drawChar(s, 0, 0, 'A', 5) == 9, "advance = width + spacing");
		CHECK(s.get(0, 0) == 5 && s.get(7, 0) == 5 && s.get(1, 1) == 0 && s.get(7, 1) == 5, "glyph pixels");
		CHECK(f.textWidth("AB") == 18, "text width");
		d.resize(12);
		CHECK(!f.load(d), "truncated font is rejected");
	}

	const char *dir = getenv("XANTH_DATA");
	Common::GameFiles gf;
	if (!dir || !gf.setFolder(dir)) {
		printf("test_graphics: XANTH_DATA not set, game files skipped\n");
		printf("test_graphics: %d checks, %d failures\n", g_checks, g_failures);
		return g_failures ? 1 : 0;
	}
	const char *shots = getenv("XANTH_SHOTS");
	static const char *kFonts[] = {"XANTH_00.FNT", "XANTH_01.FNT", "XANTH_02.FNT", "XANTH_10.FNT", "XANTH_11.FNT", "XANTH_13.FNT"};
	Legend::Surface sheet(320, 200);
	sheet.clear(0);
	byte pal[768];
	memset(pal, 0, sizeof(pal));
	pal[3] = pal[4] = pal[5] = 63;
	sheet.setPalette(pal);
	int y = 4;
	for (int i = 0; i < ARRAYSIZE(kFonts); i++) {
		Common::Bytes d;
		Legend::Font f;
		CHECK(gf.load(kFonts[i], d) && f.load(d), "%s loads", kFonts[i]);
		CHECK(f.height() > 0 && f.textWidth("Companions of Xanth") > 0, "%s measures text", kFonts[i]);
		f.drawText(sheet, 4, y, "Companions of Xanth 0123", 1);
		y += f.height() + 4;
	}
	if (shots) {
		CHECK(sheet.savePpm(std::string(shots) + "/fonts.ppm"), "write the font sheet");
	}
	{
		Xanth::MainScreen ms;
		CHECK(ms.load(gf), "the interface pictures load");
		Legend::Surface screen(320, 200);
		ms.drawFrame(screen);
		CHECK(ms.drawRoomPicture(screen, gf, 0x0200), "the bedroom picture draws");
		ms.drawCompassArrows(screen, getenv("XANTH_EXITS") ? (unsigned)atoi(getenv("XANTH_EXITS")) : 0x80u);
		static const char *kVerbs[] = {"Take", "Put", "Look at", "Open", "Close", "Talk to", "Look"};
		ms.drawVerbs(screen, std::vector<std::string>(kVerbs, kVerbs + ARRAYSIZE(kVerbs)));
		ms.printMessage(screen, "You are sitting at your desk, watching the gathering storm through your window.");
		const char *oracleShot = getenv("XANTH_ORACLE_SHOT");
		std::vector<byte> oracle;
		if (oracleShot && loadPpm(oracleShot, oracle)) {
			printf("main screen against %s:\n", oracleShot);
			std::string diffPath = shots ? std::string(shots) + "/diff.ppm" : std::string();
			compareScreens(screen, oracle, shots ? diffPath.c_str() : NULL);
		}
		if (getenv("XANTH_LABELSEARCH") && !oracle.empty()) {
			for (int fi = 0; fi < ARRAYSIZE(kFonts); fi++) {
				Common::Bytes fd;
				Legend::Font f;
				if (!gf.load(kFonts[fi], fd) || !f.load(fd))
					continue;
				int best = 1 << 30, bx = 0, by = 0;
				for (int y = 100; y < 126; y++) {
					for (int x = 130; x < 240; x++) {
						Legend::Surface t(screen);
						f.drawTextOutlined(t, x, y, "computer", 15, 0);
						std::vector<byte> rgb(320 * 200 * 3);
						t.toRgb(&rgb[0]);
						int bad = 0;
						for (int yy = 100; yy < 128; yy++)
							for (int xx = 130; xx < 250; xx++)
								bad += memcmp(&rgb[(yy * 320 + xx) * 3], &oracle[(yy * 320 + xx) * 3], 3) != 0;
						if (bad < best) {
							best = bad;
							bx = x;
							by = y;
						}
					}
				}
				printf("label search: %s best %d differing pixels at (%d,%d)\n", kFonts[fi], best, bx, by);
			}
		}
		if (shots)
			screen.savePpm(std::string(shots) + "/screen.ppm");
	}
	printf("test_graphics: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}

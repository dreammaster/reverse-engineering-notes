// Fonts and surfaces: all six XANTH_nn.FNT files parse, text measures and draws; a picture blits to a surface and converts to RGB.
// With XANTH_SHOTS=<folder> a PPM of a sample line of every font and of the first picture is written for a look.
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "legend/draw.h"
#include "legend/font.h"
#include "legend/pic.h"
#include "legend/surface.h"
#include "tests/screen_compare.h"
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
		CHECK(ms.drawRoomPicture(screen, gf, 0x0200, std::vector<int>({0x0201, 0x0202, 0x0214, 0x0215})), "the bedroom picture draws");
		ms.drawCompassArrows(screen, getenv("XANTH_EXITS") ? (unsigned)atoi(getenv("XANTH_EXITS")) : 0x80u);
		static const char *kVerbs[] = {"Take", "Put", "Look at", "Open", "Close", "Talk to", "Look"};
		ms.drawVerbs(screen, std::vector<std::string>(kVerbs, kVerbs + ARRAYSIZE(kVerbs)));
		ms.printMessage(screen, "You are sitting at your desk, watching the gathering storm through your window.");
		{
			// the hover label of the object under the pointer: outline font 1, white on black
			Common::Bytes fd;
			Legend::Font f1;
			CHECK(gf.load("XANTH_01.FNT", fd) && f1.load(fd), "font 1 loads");
			f1.drawTextOutlined(screen, 151, 114, "computer", 15, 0);
		}
		const char *oracleShot = getenv("XANTH_ORACLE_SHOT");
		std::vector<byte> oracle;
		if (oracleShot && loadPpm(oracleShot, oracle)) {
			printf("main screen against %s:\n", oracleShot);
			std::string diffPath = shots ? std::string(shots) + "/diff.ppm" : std::string();
			compareScreens(screen, oracle, shots ? diffPath.c_str() : NULL);
		}
		if (shots)
			screen.savePpm(std::string(shots) + "/screen.ppm");
	}
	printf("test_graphics: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}

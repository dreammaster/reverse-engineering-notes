/* vidshot <gameDir> <fontDir> <out.png>
 *
 * Headless check of Yendor1::Vid with real game data: draws a scene through the driver
 * calls only (tiles from FILE01/FILE03 with putImage at aligned and unaligned x, lines,
 * rects, text, the mouse cursor) and writes page 0 as a PNG. */

#include "data/Palette.h"
#include "data/Tiles.h"
#include "tools/Png.h"
#include "vid/Fonts.h"
#include "vid/Video.h"

#include <stdio.h>
#include <string.h>
#include <vector>

using namespace Yendor1;

static void putText(Vid::Video &v, int col, int row, const char *s, int fg, int bg) {
	for (; *s; s++, col++) {
		v.ctx.textCol = (Shared::byte)col;
		v.ctx.textRow = (Shared::byte)row;
		v.ctx.textChar = (Shared::byte)*s;
		v.ctx.textFg = (Shared::byte)fg;
		v.ctx.textBg = (Shared::byte)bg;
		v.drawChar();
	}
}

static void putTile(Vid::Video &v, const Data::TileFile &t, size_t index, int x, int y) {
	const Shared::byte *raw = t.raw(index);
	if (!raw)
		return;
	v.ctx.image.set(const_cast<Shared::byte *>(raw), Data::kTileBytes, 0);
	v.ctx.x1 = (Shared::uint16)x;
	v.ctx.y1 = (Shared::uint16)y;
	v.putImage();
}

int main(int argc, char **argv) {
	if (argc < 4) {
		fprintf(stderr, "usage: %s <gameDir> <fontDir> <out.png>\n", argv[0]);
		return 2;
	}
	Data::TileFile t1, t3;
	Data::PaletteFile p3;
	if (!t1.load(argv[1], 1) || !t3.load(argv[1], 3) || !p3.load(argv[1], 3)) {
		fprintf(stderr, "cannot load game graphics from %s\n", argv[1]);
		return 1;
	}
	Vid::Fonts fonts;
	const bool haveFonts = fonts.load(argv[2]);

	Vid::Video v;
	fonts.attach(v);
	v.init(true);
	v.setGraphicsMode();
	v.setPaletteRegisters(p3.record(1));

	// a tiled floor from FILE03 (aligned x), then the same tiles shifted by a few pixels
	for (int y = 0; y < 5; y++)
		for (int x = 0; x < 8; x++)
			putTile(v, t3, 20 + ((x + y) % 6), 16 + x * 32, 30 + y * 32);
	for (int x = 0; x < 6; x++)
		putTile(v, t3, 40 + x, 300 + x * 35 + (x & 3), 70 + x * 8);   // unaligned x positions

	v.ctx.colour = 14;
	v.ctx.x1 = 10; v.ctx.y1 = 10; v.ctx.x2 = 629; v.ctx.y2 = 339;
	v.rect();
	v.ctx.colour = 12;
	v.ctx.x1 = 20; v.ctx.y1 = 300; v.ctx.x2 = 200; v.ctx.y2 = 340;
	v.line();
	v.ctx.x1 = 20; v.ctx.y1 = 340; v.ctx.x2 = 200; v.ctx.y2 = 300;
	v.line();
	v.ctx.colour = 1;
	v.ctx.x1 = 220; v.ctx.y1 = 300; v.ctx.x2 = 420; v.ctx.y2 = 335;
	v.fillRect();
	if (haveFonts) {
		putText(v, 28, 22, "Yendor1::Vid", 15, 1);
		putText(v, 28, 23, "FILE03 tiles, putImage", 14, 1);
		v.ctx.fontSelect = 1;
		putText(v, 28, 33, "8x8 font row", 11, 0);
	}
	// portrait tiles from FILE01 (each portrait is a block of tiles)
	for (int y = 0; y < 3; y++)
		for (int x = 0; x < 3; x++)
			putTile(v, t1, y * 20 + x, 440 + x * 32, 190 + y * 32);

	std::vector<Shared::byte> rgb((size_t)Vid::kScreenWidth * Vid::kScreenHeight * 3);
	v.renderRgb(0, &rgb[0]);
	if (!Tools::writePng(argv[3], Vid::kScreenWidth, Vid::kScreenHeight, &rgb[0])) {
		fprintf(stderr, "cannot write %s\n", argv[3]);
		return 1;
	}
	printf("%s written (fonts %s)\n", argv[3], haveFonts ? "loaded" : "MISSING: text skipped");
	return 0;
}

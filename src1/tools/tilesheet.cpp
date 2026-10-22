/* tilesheet <gameDir> <fileNumber> <firstTile> <count> <paletteRecord> <out.png> [columns]
 *
 * Render tiles of FILE0n.16 to a PNG contact sheet using a record of FILE0n.PAL.
 * Used to inspect the graphics and to check the tile/palette decoders by eye. */

#include "data/Palette.h"
#include "data/Tiles.h"
#include "tools/Png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace Yendor1;

int main(int argc, char **argv) {
	if (argc < 7) {
		fprintf(stderr, "usage: %s <gameDir> <file 0-6> <firstTile> <count> <paletteRecord> <out.png> [columns]\n", argv[0]);
		return 2;
	}
	const char *dir = argv[1];
	const int fileNo = atoi(argv[2]);
	const size_t first = (size_t)atoi(argv[3]);
	size_t count = (size_t)atoi(argv[4]);
	const size_t palRec = (size_t)atoi(argv[5]);
	const int cols = argc > 7 ? atoi(argv[7]) : 20;

	Data::TileFile tiles;
	Data::PaletteFile pal;
	if (!tiles.load(dir, fileNo) || !pal.load(dir, fileNo)) {
		fprintf(stderr, "cannot load FILE%02d.16/.PAL from %s\n", fileNo, dir);
		return 1;
	}
	const Shared::byte *regs = pal.record(palRec);
	if (!regs) {
		fprintf(stderr, "palette record %zu out of range (%zu records)\n", palRec, pal.count());
		return 1;
	}
	if (first >= tiles.count())
		return 1;
	if (first + count > tiles.count())
		count = tiles.count() - first;

	Data::Rgb colours[16];
	for (int i = 0; i < 16; i++)
		colours[i] = Data::egaToRgb(regs[i]);

	const int w = cols * Data::kTileWidth;
	const int rows = (int)((count + cols - 1) / cols);
	const int h = rows * Data::kTileHeight;
	std::vector<uint8_t> img((size_t)w * h * 3, 0);
	Shared::byte px[Data::kTileWidth * Data::kTileHeight];
	for (size_t i = 0; i < count; i++) {
		tiles.decode(first + i, px);
		const int ox = (int)(i % cols) * Data::kTileWidth;
		const int oy = (int)(i / cols) * Data::kTileHeight;
		for (int y = 0; y < Data::kTileHeight; y++) {
			for (int x = 0; x < Data::kTileWidth; x++) {
				const Data::Rgb &c = colours[px[y * Data::kTileWidth + x]];
				uint8_t *d = &img[((size_t)(oy + y) * w + ox + x) * 3];
				d[0] = c.r;
				d[1] = c.g;
				d[2] = c.b;
			}
		}
	}
	if (!Tools::writePng(argv[6], w, h, &img[0])) {
		fprintf(stderr, "cannot write %s\n", argv[6]);
		return 1;
	}
	printf("%s: %d x %d, %zu tiles (file %d from %zu), palette record %zu of %zu\n", argv[6], w, h, count, fileNo, first, palRec, pal.count());
	return 0;
}

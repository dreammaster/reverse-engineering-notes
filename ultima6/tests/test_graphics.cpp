// Library archives, shapes, the tile set and the tile descriptions against the real Ultima VI files
// (skipped when the game data is absent).

#include "files/lib_n.h"
#include "files/lzw.h"
#include "files/shape.h"
#include "files/tile_data.h"
#include "files/tile_set.h"

#include <cstdio>

using namespace Ultima6;

static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                         \
		if (!(cond)) {                                                           \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
			++failures;                                                          \
		}                                                                        \
	} while (0)

static bool load(const char *name, Bytes &data) {
	return readFile(joinPath("u6/game", name), data);
}

static bool loadLzw(const char *name, Bytes &out) {
	Bytes raw;
	return load(name, raw) && lzwDecompress(raw, 0, out);
}

int main() {
	Bytes raw;
	if (!load("TILEINDX.VGA", raw)) {
		std::printf("skip (no game data)\n");
		return 0;
	}

	// Tiles: all 2048 decode; the kinds are the 439 / 177 / 1432 split and a checksum of
	// every pixel matches an independent decoder.
	Bytes mapTiles, maskType, objTiles, index;
	CHECK(loadLzw("MAPTILES.VGA", mapTiles) && loadLzw("MASKTYPE.VGA", maskType));
	CHECK(load("OBJTILES.VGA", objTiles) && load("TILEINDX.VGA", index));
	static TileSet tiles;
	CHECK(tiles.load(mapTiles, maskType, objTiles, index));
	int plain = 0, trans = 0, blocks = 0, empty = 0;
	uint32_t sum = 0;
	for (int t = 0; t < TileSet::COUNT; ++t) {
		plain += tiles.kind[t] == TileSet::PLAIN;
		trans += tiles.kind[t] == TileSet::TRANSPARENT;
		blocks += tiles.kind[t] == TileSet::PIXEL_BLOCKS;
		bool hasTransparent = false, hasOpaque = false;
		for (int i = 0; i < TileSet::SIZE; ++i) {
			sum += tiles.pixels[t][i];
			(tiles.pixels[t][i] == TileSet::TRANSPARENT_PIXEL ? hasTransparent : hasOpaque) = true;
		}
		empty += tiles.kind[t] == TileSet::PIXEL_BLOCKS && !hasOpaque;
	}
	CHECK(plain == 439 && trans == 177 && blocks == 1432);
	CHECK(sum == 74253342u);
	CHECK(empty < 200); // blank object tiles exist, but most pixel-block tiles draw something
	std::printf("%d pixel-block tiles are blank\n", empty);

	// Descriptions.
	Bytes look;
	LookData desc;
	CHECK(loadLzw("LOOK.LZD", look) && desc.load(look));
	CHECK(desc.description(0) == "Nothing");
	CHECK(desc.description(1) == "grass");
	CHECK(desc.description(3) == "swamp"); // tiles 2-4 share the entry numbered 4
	CHECK(desc.description(1791) == "Avatar");

	// Library archives.
	LibN lib;
	CHECK(load("PORTRAIT.A", raw) && lib.load(raw, false));
	CHECK(lib.count() == 98);
	size_t n;
	CHECK(lib.item(97, n) != nullptr && n > 0);
	CHECK(load("CONVERSE.A", raw) && lib.load(raw, false));
	CHECK(lib.count() == 99 && !lib.present(0));

	// Shape files: every shape decodes; counts and the portrait size.
	struct { const char *name; size_t count; } files[] = {
		{"MONTAGE.SHP", 15}, {"MAINMENU.SHP", 1}, {"TITLES.SHP", 2}, {"BLOCKS.SHP", 4}, {"INTRO.SHP", 13},
		{"VELLUM1.SHP", 19}, {"GYPSY.SHP", 200}, {"END.SHP", 13}, {"WOODS.SHP", 2}, {"INTRO_1.SHP", 71},
		{"INTRO_2.SHP", 29}, {"INTRO_3.SHP", 27},
	};
	for (const auto &f : files) {
		std::vector<Shape> shapes;
		CHECK(load(f.name, raw) && loadShapeFile(raw, shapes));
		CHECK(shapes.size() == f.count);
		if (std::string(f.name) == "MONTAGE.SHP") {
			CHECK(shapes[0].width == 56 && shapes[0].height == 64);
		}
		if (std::string(f.name) == "INTRO.SHP") {
			bool full = false;
			for (const Shape &sh : shapes)
				full |= sh.width == 320 && sh.height == 200;
			CHECK(full);
		}
	}

	// Cursors are shape files too; the font and the intro palettes.
	for (const char *name : {"U6MCGA.PTR", "INTRO.PTR"}) {
		std::vector<Shape> shapes;
		CHECK(load(name, raw) && loadShapeFile(raw, shapes) && !shapes.empty());
	}
	Font font;
	CHECK(load("U6.CH", raw) && font.load(raw));
	bool blank = true, drawn = false;
	for (int r = 0; r < Font::ROWS; ++r) {
		blank &= font.glyph[' '][r] == 0;
		drawn |= font.glyph['A'][r] != 0;
	}
	CHECK(blank && drawn);
	IntroPalettes pals;
	CHECK(load("PALETTES.INT", raw) && pals.load(raw));

	if (failures == 0)
		std::printf("ok\n");
	return failures ? 1 : 0;
}

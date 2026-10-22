/* Tile and palette decoders: synthetic cases plus the real game files. */

#include "TestUtil.h"
#include "data/Palette.h"
#include "data/Tiles.h"
#include "shared/Bytes.h"

using namespace Yendor1;

static void testBytes() {
	const Shared::byte b[4] = {0x34, 0x12, 0xFF, 0xFF};
	CHECK_EQ(Shared::readLE16(b), 0x1234);
	CHECK_EQ(Shared::readLE16s(b + 2), -1);
	CHECK_EQ(Shared::readLE32(b), 0xFFFF1234u);
	Shared::byte o[4];
	Shared::writeLE32(o, 0xDEADBEEFu);
	CHECK_EQ(Shared::readLE32(o), 0xDEADBEEFu);
}

static void testTileDecodeSynthetic() {
	// plane 3 (first in the file): leftmost pixel of row 0 set; plane 0 (last): rightmost pixel of row 31.
	Shared::byte raw[Data::kTileBytes] = {0};
	raw[0 * Data::kTilePlaneBytes + 0] = 0x80;                  // plane 3, row 0, x=0
	raw[3 * Data::kTilePlaneBytes + 31 * 4 + 3] = 0x01;         // plane 0, row 31, x=31
	raw[1 * Data::kTilePlaneBytes + 5 * 4 + 1] = 0x40;          // plane 2, row 5, x=9
	raw[2 * Data::kTilePlaneBytes + 5 * 4 + 1] = 0x40;          // plane 1, row 5, x=9
	Shared::byte px[1024];
	Data::TileFile::decodeRaw(raw, px);
	CHECK_EQ(px[0], 8);
	CHECK_EQ(px[31 * 32 + 31], 1);
	CHECK_EQ(px[5 * 32 + 9], 6);
	int nonzero = 0;
	for (int i = 0; i < 1024; i++)
		nonzero += px[i] != 0;
	CHECK_EQ(nonzero, 3);

	Data::TileFile tf;
	tf.assign(raw, sizeof(raw));
	CHECK_EQ(tf.count(), 1u);
	CHECK(tf.raw(1) == 0);
	CHECK(tf.decode(0, px));
	CHECK(!tf.decode(1, px));
}

static void testEgaColours() {
	Data::Rgb c = Data::egaToRgb(0x00);
	CHECK_EQ(c.r, 0); CHECK_EQ(c.g, 0); CHECK_EQ(c.b, 0);
	c = Data::egaToRgb(0x3F);   // all bits: white
	CHECK_EQ(c.r, 255); CHECK_EQ(c.g, 255); CHECK_EQ(c.b, 255);
	c = Data::egaToRgb(0x04);   // red primary only
	CHECK_EQ(c.r, 170); CHECK_EQ(c.g, 0); CHECK_EQ(c.b, 0);
	c = Data::egaToRgb(0x38);   // the three secondary bits: the "dark grey" of EGA register 7
	CHECK_EQ(c.r, 85); CHECK_EQ(c.g, 85); CHECK_EQ(c.b, 85);
}

static void testPaletteSynthetic() {
	Shared::byte raw[40] = {0};
	for (int i = 0; i < 17; i++) raw[i] = (Shared::byte)i;
	for (int i = 17; i < 34; i++) raw[i] = (Shared::byte)(0x3F - (i - 17));
	Data::PaletteFile p;
	raw[16] = 0; raw[33] = 0;                // spare bytes
	p.assign(raw, 40);                       // 40 bytes: two complete records, 6 stale bytes
	CHECK_EQ(p.count(), 2u);
	raw[17 + 5] = 0x77;                      // an out-of-range register value ends the valid run
	p.assign(raw, 40);
	CHECK_EQ(p.count(), 1u);
	raw[17 + 5] = 0x10;
	p.assign(raw, 40);
	CHECK_EQ(p.record(0)[3], 3);
	CHECK_EQ(p.record(1)[0], 0x3F);
	CHECK(p.record(2) == 0);
}

static void testRealFiles() {
	if (!TestUtil::haveGameFiles()) {
		printf("  skipped (no game files)\n");
		return;
	}
	static const size_t tileCounts[7] = {1520, 1440, 1040, 960, 1200, 720, 1440};
	static const size_t palCounts[7] = {19, 18, 13, 12, 15, 9, 18};   // FILE03.PAL: 25 records' worth of bytes, 12 valid
	for (int i = 0; i < 7; i++) {
		Data::TileFile t;
		Data::PaletteFile p;
		CHECK(t.load(TestUtil::gameDir(), i));
		CHECK(p.load(TestUtil::gameDir(), i));
		CHECK_EQ(t.count(), tileCounts[i]);
		CHECK_EQ(p.count(), palCounts[i]);
		// every palette register value is a 6-bit value
		for (size_t r = 0; r < p.count(); r++)
			for (int c = 0; c < 16; c++)
				CHECK(p.record(r)[c] <= 0x3F);
	}
	// FILE00 tile 0: a few sparkle pixels (known non-zero bytes at plane 3 offset 71, plane 2 offset 195 ...)
	Data::TileFile t0;
	CHECK(t0.load(TestUtil::gameDir(), 0));
	Shared::byte px[1024];
	CHECK(t0.decode(0, px));
	CHECK_EQ(px[17 * 32 + 26], 8);          // plane 3 row 17 byte 3 = 0x20: pixel x=26
	int lit = 0;
	for (int i = 0; i < 1024; i++) lit += px[i] != 0;
	CHECK(lit > 0 && lit < 100);
}

TEST_MAIN_BEGIN("data graphics")
	testBytes();
	testTileDecodeSynthetic();
	testEgaColours();
	testPaletteSynthetic();
	testRealFiles();
TEST_MAIN_END()

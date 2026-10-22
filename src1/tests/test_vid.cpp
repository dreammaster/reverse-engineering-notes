/* Yendor1::Vid: the driver's drawing primitives, checked against hand-worked results
 * from the disassembly (docs1/video-api.md). */

#include "TestUtil.h"
#include "vid/Video.h"
#include "data/Tiles.h"

#include <string.h>
#include <vector>

using namespace Yendor1;
using Vid::Video;
using Shared::byte;

static int countPixels(const Video &v, int page, byte colour) {
	int n = 0;
	for (int y = 0; y < Vid::kScreenHeight; y++)
		for (int x = 0; x < Vid::kScreenWidth; x++)
			n += v.pixel(page, x, y) == colour;
	return n;
}

static void testPixels() {
	Video v;
	v.init(false);
	v.ctx.colour = 9;
	v.ctx.x1 = 13; v.ctx.y1 = 7;
	v.putPixel();
	CHECK_EQ(v.pixel(0, 13, 7), 9);
	CHECK_EQ(v.pixel(0, 12, 7), 0);
	CHECK_EQ(countPixels(v, 0, 9), 1);
	// plane layout: colour 9 = planes 0 and 3; pixel 13 is bit 7-5=2 of byte 1 in row 7
	CHECK_EQ(v.plane(0, 7 * 80 + 1), 1 << 2);
	CHECK_EQ(v.plane(3, 7 * 80 + 1), 1 << 2);
	CHECK_EQ(v.plane(1, 7 * 80 + 1), 0);
	v.ctx.colour = 0;
	v.getPixel();
	CHECK_EQ(v.ctx.colour, 9);
	// page 1
	v.ctx.drawPage = 1;
	v.ctx.colour = 5; v.ctx.x1 = 639; v.ctx.y1 = 349;
	v.putPixel();
	CHECK_EQ(v.pixel(1, 639, 349), 5);
	CHECK_EQ(v.pixel(0, 639, 349), 0);
	// overwriting replaces all four planes of the pixel
	v.ctx.colour = 2;
	v.putPixel();
	CHECK_EQ(v.pixel(1, 639, 349), 2);
}

static void testLines() {
	Video v;
	v.init(false);
	v.ctx.colour = 4;
	// horizontal, unaligned both ends
	v.ctx.x1 = 5; v.ctx.y1 = 10; v.ctx.x2 = 22; v.ctx.y2 = 10;
	v.line();
	CHECK_EQ(countPixels(v, 0, 4), 18);
	CHECK_EQ(v.pixel(0, 4, 10), 0);
	CHECK_EQ(v.pixel(0, 5, 10), 4);
	CHECK_EQ(v.pixel(0, 22, 10), 4);
	CHECK_EQ(v.pixel(0, 23, 10), 0);
	// vertical
	v.ctx.x1 = 100; v.ctx.y1 = 20; v.ctx.x2 = 100; v.ctx.y2 = 29;
	v.line();
	CHECK_EQ(countPixels(v, 0, 4), 28);
	CHECK_EQ(v.pixel(0, 100, 29), 4);
	CHECK_EQ(v.pixel(0, 100, 30), 0);
	// a vertical line given bottom-to-top runs DOWN from (x1,y1) -- a quirk of the original
	Video w;
	w.init(false);
	w.ctx.colour = 3;
	w.ctx.x1 = 50; w.ctx.y1 = 40; w.ctx.x2 = 50; w.ctx.y2 = 35;
	w.line();
	CHECK_EQ(w.pixel(0, 50, 40), 3);
	CHECK_EQ(w.pixel(0, 50, 45), 3);
	CHECK_EQ(w.pixel(0, 50, 35), 0);
	CHECK_EQ(countPixels(w, 0, 3), 6);
	// diagonal and endpoints get swapped in the context when x2 < x1
	Video d;
	d.init(false);
	d.ctx.colour = 7;
	d.ctx.x1 = 30; d.ctx.y1 = 30; d.ctx.x2 = 20; d.ctx.y2 = 20;
	d.line();
	CHECK_EQ(d.ctx.x1, 20); CHECK_EQ(d.ctx.x2, 30);
	CHECK_EQ(d.ctx.y1, 20); CHECK_EQ(d.ctx.y2, 30);
	for (int i = 0; i <= 10; i++)
		CHECK_EQ(d.pixel(0, 20 + i, 20 + i), 7);
	CHECK_EQ(countPixels(d, 0, 7), 11);
	// shallow slope: every x gets exactly one pixel, endpoints included
	Video s;
	s.init(false);
	s.ctx.colour = 1;
	s.ctx.x1 = 0; s.ctx.y1 = 0; s.ctx.x2 = 40; s.ctx.y2 = 10;
	s.line();
	CHECK_EQ(countPixels(s, 0, 1), 41);
	CHECK_EQ(s.pixel(0, 0, 0), 1);
	CHECK_EQ(s.pixel(0, 40, 10), 1);
	for (int x = 0; x <= 40; x++) {
		int n = 0;
		for (int y = 0; y <= 10; y++) n += s.pixel(0, x, y) == 1;
		CHECK_EQ(n, 1);
	}
	// steep slope: every y gets exactly one pixel, going up
	Video t;
	t.init(false);
	t.ctx.colour = 6;
	t.ctx.x1 = 10; t.ctx.y1 = 60; t.ctx.x2 = 20; t.ctx.y2 = 20;
	t.line();
	CHECK_EQ(countPixels(t, 0, 6), 41);
	CHECK_EQ(t.pixel(0, 10, 60), 6);
	CHECK_EQ(t.pixel(0, 20, 20), 6);
}

static void testRects() {
	Video v;
	v.init(false);
	v.ctx.colour = 2;
	v.ctx.x1 = 10; v.ctx.y1 = 10; v.ctx.x2 = 19; v.ctx.y2 = 19;
	v.rect();
	CHECK_EQ(countPixels(v, 0, 2), 36);          // 10x10 outline
	CHECK_EQ(v.pixel(0, 10, 10), 2);
	CHECK_EQ(v.pixel(0, 19, 19), 2);
	CHECK_EQ(v.pixel(0, 15, 15), 0);
	CHECK_EQ(v.ctx.x1, 10); CHECK_EQ(v.ctx.y2, 19);   // fields restored
	Video f;
	f.init(false);
	f.ctx.colour = 12;
	f.ctx.x1 = 3; f.ctx.y1 = 4; f.ctx.x2 = 66; f.ctx.y2 = 8;
	f.fillRect();
	CHECK_EQ(countPixels(f, 0, 12), 64 * 5);
	CHECK_EQ(f.pixel(0, 2, 4), 0);
	CHECK_EQ(f.pixel(0, 3, 4), 12);
	CHECK_EQ(f.pixel(0, 66, 8), 12);
	CHECK_EQ(f.pixel(0, 67, 8), 0);
	CHECK_EQ(f.ctx.y1, 4); CHECK_EQ(f.ctx.y2, 8);
}

static void testText() {
	// a 3-row glyph font for char 'A' (index 65): pattern rows 0xF0, 0x80, 0xFF
	static byte font[256 * 8];
	memset(font, 0, sizeof(font));
	font[65 * 8 + 0] = 0xF0;
	font[65 * 8 + 1] = 0x80;
	font[65 * 8 + 2] = 0xFF;
	Video v;
	v.init(false);
	v.setFont8x8(font);
	v.ctx.fontSelect = 1;
	v.ctx.textCol = 3; v.ctx.textRow = 2; v.ctx.textChar = 65;
	v.ctx.textFg = 14; v.ctx.textBg = 1;
	v.drawChar();
	// x = 24, y = 16; row 0 = 0xF0: first 4 pixels fg, last 4 bg
	CHECK_EQ(v.pixel(0, 24, 16), 14);
	CHECK_EQ(v.pixel(0, 27, 16), 14);
	CHECK_EQ(v.pixel(0, 28, 16), 1);
	CHECK_EQ(v.pixel(0, 31, 16), 1);
	CHECK_EQ(v.pixel(0, 24, 17), 14);
	CHECK_EQ(v.pixel(0, 25, 17), 1);
	CHECK_EQ(v.pixel(0, 31, 18), 14);
	CHECK_EQ(v.pixel(0, 23, 16), 0);      // neighbours untouched
	CHECK_EQ(v.pixel(0, 24, 24), 0);      // below the 8-row glyph
}

// A 32x32 tile with a recognisable pattern, as raw planar bytes.
static void makeTile(byte *raw, int seed) {
	for (int i = 0; i < 512; i++)
		raw[i] = (byte)(i * 37 + seed * 11 + (i >> 3));
}

static void testPutImageAligned() {
	byte raw[512];
	makeTile(raw, 1);
	Video v;
	v.init(false);
	v.ctx.image.set(raw, sizeof(raw), 0);
	v.ctx.x1 = 64; v.ctx.y1 = 40;
	v.putImage();
	byte px[1024];
	Data::TileFile::decodeRaw(raw, px);
	int bad = 0;
	for (int y = 0; y < 32; y++)
		for (int x = 0; x < 32; x++)
			bad += v.pixel(0, 64 + x, 40 + y) != px[y * 32 + x];
	CHECK_EQ(bad, 0);
	CHECK_EQ(v.pixel(0, 63, 40), 0);
	CHECK_EQ(v.pixel(0, 96, 40), 0);
	CHECK_EQ(v.pixel(0, 64, 72), 0);
}

static void testPutImageUnaligned() {
	byte raw[512];
	makeTile(raw, 2);
	byte px[1024];
	Data::TileFile::decodeRaw(raw, px);
	for (int dx = 1; dx < 8; dx++) {
		Video v;
		v.init(false);
		// a background the image must not disturb outside its rectangle
		for (int p = 0; p < 4; p++)
			for (int i = 0; i < Vid::kPageBytes; i++)
				v.setPlane(p, (Shared::uint16)i, 0xFF);
		v.ctx.image.set(raw, sizeof(raw), 0);
		v.ctx.x1 = (Shared::uint16)(64 + dx); v.ctx.y1 = 40;
		v.putImage();
		int bad = 0, outside = 0;
		for (int y = 0; y < 32; y++)
			for (int x = 0; x < 32; x++)
				bad += v.pixel(0, 64 + dx + x, 40 + y) != px[y * 32 + x];
		for (int y = 36; y < 76; y++)
			for (int x = 56; x < 110; x++) {
				const bool inside = x >= 64 + dx && x < 96 + dx && y >= 40 && y < 72;
				if (!inside && v.pixel(0, x, y) != 15)
					outside++;
			}
		CHECK_EQ(bad, 0);
		CHECK_EQ(outside, 0);
	}
}

static void testPutImageMask() {
	// a 12-pixel-wide image (2 bytes per plane row), right-edge mask F0: only 12 pixels are drawn
	byte raw[4 * 4 * 2];
	memset(raw, 0xFF, sizeof(raw));
	for (int dx = 0; dx < 8; dx++) {
		Video v;
		v.init(false);
		v.ctx.image.set(raw, sizeof(raw), 0);
		v.ctx.imgRows = 4; v.ctx.imgBytes = 2; v.ctx.imgMask = 0xF0;
		v.ctx.x1 = (Shared::uint16)(8 + dx); v.ctx.y1 = 5;
		v.putImage();
		int n = 0;
		for (int y = 0; y < 12; y++)
			for (int x = 0; x < 40; x++)
				if (v.pixel(0, x, y) != 0) {
					n++;
					CHECK(y >= 5 && y < 9 && x >= 8 + dx && x < 20 + dx);
				}
		CHECK_EQ(n, 12 * 4);
	}
}

static void testGetPutRoundTrip() {
	// fn0B captures a rectangle with a 5-byte header; fn0C draws it back (any x alignment)
	for (int x0 = 0; x0 < 8; x0++) {
		Video v;
		v.init(false);
		// pattern: draw some lines
		v.ctx.colour = 13;
		v.ctx.x1 = 21; v.ctx.y1 = 11; v.ctx.x2 = 60; v.ctx.y2 = 30;
		v.line();
		v.ctx.colour = 6;
		v.ctx.x1 = 25; v.ctx.y1 = 12; v.ctx.x2 = 25; v.ctx.y2 = 28;
		v.line();
		static byte buf[2048];
		memset(buf, 0, sizeof(buf));
		v.ctx.image.set(buf, sizeof(buf), 0);
		v.ctx.x1 = (Shared::uint16)(16 + x0); v.ctx.y1 = 10;
		v.ctx.x2 = (Shared::uint16)(16 + x0 + 44); v.ctx.y2 = 33;       // 45 x 24
		v.getImage();
		CHECK_EQ(buf[0], 24);
		CHECK_EQ(buf[2], 6);                       // ceil(45/8) bytes per row
		CHECK_EQ(buf[4], (byte)(0xFF << (7 - (44 & 7))));
		// clear and draw back at the same place
		Video w;
		w.init(false);
		w.ctx.image.set(buf, sizeof(buf), 0);
		w.ctx.x1 = (Shared::uint16)(16 + x0); w.ctx.y1 = 10;
		w.putImageHdr();
		int bad = 0;
		for (int y = 10; y < 34; y++)
			for (int x = 16 + x0; x < 16 + x0 + 45; x++)
				bad += w.pixel(0, x, y) != v.pixel(0, x, y);
		CHECK_EQ(bad, 0);
		CHECK_EQ(w.ctx.image.offset, 5u);            // the stream pointer advanced past the header
		CHECK_EQ(w.ctx.imgRows, 0x20);               // size fields reset
	}
}

static void testRawGetAlignedRoundTrip() {
	byte raw[512];
	makeTile(raw, 3);
	Video v;
	v.init(false);
	v.ctx.image.set(raw, sizeof(raw), 0);
	v.ctx.x1 = 32; v.ctx.y1 = 64;
	v.putImage();
	byte out[512];
	memset(out, 0, sizeof(out));
	v.ctx.image.set(out, sizeof(out), 0);
	v.getImageRaw();
	CHECK(memcmp(raw, out, 512) == 0);
}

static void testCopies() {
	Video v;
	v.init(false);
	v.ctx.colour = 11;
	v.ctx.x1 = 0; v.ctx.y1 = 0; v.ctx.x2 = 639; v.ctx.y2 = 349;
	v.fillRect();
	v.ctx.colour = 3;
	v.ctx.x1 = 100; v.ctx.y1 = 100; v.ctx.x2 = 140; v.ctx.y2 = 120;
	v.rect();
	v.copyScreen0To1AndShow();
	CHECK_EQ(v.displayedPage(), 1);
	int bad = 0;
	for (int y = 0; y < 350; y++)
		for (int x = 0; x < 640; x++)
			bad += v.pixel(1, x, y) != v.pixel(0, x, y);
	CHECK_EQ(bad, 0);
	// change page 1 and copy back
	v.ctx.drawPage = 1;
	v.ctx.colour = 8;
	v.ctx.x1 = 5; v.ctx.y1 = 5; v.ctx.x2 = 5; v.ctx.y2 = 5;
	v.putPixel();
	v.copyScreen1To0AndShow();
	CHECK_EQ(v.displayedPage(), 0);
	CHECK_EQ(v.pixel(0, 5, 5), 8);
	// tile row copy: 3 tiles = 96 pixels from (32,100) page 0 to (200,40) page 1
	Video w;
	w.init(false);
	w.ctx.colour = 5;
	w.ctx.x1 = 32; w.ctx.y1 = 100; w.ctx.x2 = 127; w.ctx.y2 = 100;
	w.line();
	w.ctx.x1 = 32; w.ctx.y1 = 100; w.ctx.x3 = 200; w.ctx.y3 = 40; w.ctx.drawPage = 0; w.ctx.destPage = 1;
	w.copyTileRow(3);
	for (int i = 0; i < 96; i++)
		CHECK_EQ(w.pixel(1, 200 + i, 40), 5);
	CHECK_EQ(w.pixel(1, 296, 40), 0);
	CHECK_EQ(w.pixel(1, 199, 40), 0);
}

static void testKeyedCopy() {
	Video v;
	v.init(false);
	// 4x3 source block at (8,8): colour 6 with a hole of colour 0 at (9,9)
	v.ctx.colour = 6;
	v.ctx.x1 = 8; v.ctx.y1 = 8; v.ctx.x2 = 11; v.ctx.y2 = 10;
	v.fillRect();
	v.ctx.colour = 0;
	v.ctx.x1 = 9; v.ctx.y1 = 9;
	v.putPixel();
	// destination already holds colour 2
	v.ctx.colour = 2;
	v.ctx.x1 = 30; v.ctx.y1 = 30; v.ctx.x2 = 40; v.ctx.y2 = 40;
	v.fillRect();
	// x2/y2 are exclusive: copy a 4x3 block
	v.ctx.x1 = 8; v.ctx.y1 = 8; v.ctx.x2 = 12; v.ctx.y2 = 11;
	v.ctx.x3 = 32; v.ctx.y3 = 32;
	v.ctx.keyColour = 0;
	v.copyRectKeyed();
	CHECK_EQ(v.pixel(0, 32, 32), 6);
	CHECK_EQ(v.pixel(0, 35, 34), 6);
	CHECK_EQ(v.pixel(0, 33, 33), 2);     // the keyed pixel left the destination alone
	CHECK_EQ(v.pixel(0, 36, 32), 2);     // outside the 4-wide block
	CHECK_EQ(v.ctx.x1, 8);               // x restored, y advanced
	CHECK_EQ(v.ctx.y1, 11);
}

static void testReveals() {
	Video v;
	v.init(false);
	v.ctx.colour = 9;
	v.ctx.x1 = 0; v.ctx.y1 = 0; v.ctx.x2 = 639; v.ctx.y2 = 349;
	v.fillRect();
	v.revealCenterOut0To1();
	CHECK_EQ(v.pixel(1, 0, 0), 9);
	CHECK_EQ(v.pixel(1, 639, 349), 9);
	Video w;
	w.init(false);
	w.ctx.colour = 4;
	w.ctx.x1 = 0; w.ctx.y1 = 0; w.ctx.x2 = 639; w.ctx.y2 = 349;
	w.fillRect();
	w.revealRightToLeft0To1();
	CHECK_EQ(w.pixel(1, 639, 0), 4);
	CHECK_EQ(w.pixel(1, 8, 100), 4);
	CHECK_EQ(w.pixel(1, 0, 0), 0);       // column 0 is never copied by the original
	Video e;
	e.init(false);
	e.ctx.colour = 4;
	e.ctx.x1 = 0; e.ctx.y1 = 0; e.ctx.x2 = 639; e.ctx.y2 = 349;
	e.fillRect();
	e.revealEdgesIn0To1();
	CHECK_EQ(e.pixel(1, 0, 0), 4);
	CHECK_EQ(e.pixel(1, 639, 349), 4);
}

static void testCursorShapes() {
	Video v;
	v.init(true);
	v.ctx.cursorMode = 0;
	v.ctx.mouseX = 300; v.ctx.mouseY = 100;
	v.mouseSetCursorMode(2);
	CHECK_EQ(v.ctx.cursorMode, 2);
	// walking cursor: pointer in the central column band (bh=8) above the middle row -> shape 4
	v.ctx.mouseX = (Shared::int16)(0x30 + 8 * 32 + 5);
	v.ctx.mouseY = (Shared::int16)(0x0E + 1 * 32);
	v.mouseEvent(true, 0, 0, 0, 0);
	CHECK_EQ(v.ctx.cursorShape, 4);
	v.ctx.mouseY = (Shared::int16)(0x0E + 6 * 32);
	v.mouseEvent(true, 0, 0, 0, 0);
	CHECK_EQ(v.ctx.cursorShape, 6);
	v.ctx.mouseY = (Shared::int16)(0x0E + 4 * 32 + 3);
	v.ctx.mouseX = (Shared::int16)(0x30 + 3 * 32);
	v.mouseEvent(true, 0, 0, 0, 0);
	CHECK_EQ(v.ctx.cursorShape, 0x0C);
	v.ctx.mouseX = (Shared::int16)(0x30 + 12 * 32);
	v.mouseEvent(true, 0, 0, 0, 0);
	CHECK_EQ(v.ctx.cursorShape, 0x0A);
	// outside the play area: arrow
	v.ctx.mouseX = 10;
	v.mouseEvent(true, 0, 0, 0, 0);
	CHECK_EQ(v.ctx.cursorShape, 1);
	// button flags and clamping
	v.mouseEvent(false, 0, 0, 1, 0);
	CHECK_EQ(v.ctx.mouseFlags & 0x60, 0x60);
	v.ctx.mouseFlags = 0;
	v.mouseEvent(true, 5000, -5000, 0, 1);
	CHECK_EQ(v.ctx.mouseX, v.ctx.clipX2);
	CHECK_EQ(v.ctx.mouseY, v.ctx.clipY1);
	CHECK_EQ(v.ctx.mouseFlags, 0x80 | 0x42);
}

static void testCursorDrawing() {
	Video v;
	v.init(true);
	// shape 1: a 16x16 image whose single pixel (0,0) has colour 7 (plane bits 0,1,2)
	byte *slot = v.cursorStore() + 1 * Vid::kCursorSlotBytes;
	slot[0] = 16; slot[1] = 0; slot[2] = 2; slot[3] = 0; slot[4] = 0xFF;
	// planes 3,2,1,0 each 16 rows x 2 bytes; plane 3 empty, planes 2..0: bit 7 of row 0
	slot[5 + 0 * 32] = 0;
	slot[5 + 1 * 32] = 0x80;
	slot[5 + 2 * 32] = 0x80;
	slot[5 + 3 * 32] = 0x80;
	v.ctx.colour = 3;
	v.ctx.x1 = 0; v.ctx.y1 = 0; v.ctx.x2 = 639; v.ctx.y2 = 349;
	v.fillRect();
	v.ctx.mouseX = 100; v.ctx.mouseY = 50;
	v.ctx.cursorMode = 0;
	v.ctx.cursorShape = 1;
	v.mouseShow();                       // counter -1 -> 0: the cursor appears
	CHECK_EQ(v.pixel(0, 100, 50), 7);
	CHECK_EQ(v.pixel(0, 101, 50), 3);    // the transparent (colour 0) cursor pixels keep the background
	// move: the old position is restored, the new one drawn
	v.ctx.mouseX = 200; v.ctx.mouseY = 80;
	v.ctx.mouseFlags |= 0x80;
	v.mouseRefresh();
	CHECK_EQ(v.pixel(0, 100, 50), 3);
	CHECK_EQ(v.pixel(0, 200, 80), 7);
	CHECK_EQ(v.ctx.mouseFlags & 0x80, 0);
	v.mouseHide();
	CHECK_EQ(v.pixel(0, 200, 80), 3);
	CHECK_EQ(v.ctx.cursorHide, -1);
}

static void testDispatch() {
	Video v;
	v.init(false);
	v.ctx.colour = 5; v.ctx.x1 = 1; v.ctx.y1 = 1;
	v.call(0x03);
	CHECK_EQ(v.pixel(0, 1, 1), 5);
	v.call(0x36, 0x13);                       // PROG10's call with an unimplemented number: ignored
	v.call(0x12, 1);
	CHECK_EQ(v.displayedPage(), 1);
}

TEST_MAIN_BEGIN("vid")
	testPixels();
	testLines();
	testRects();
	testText();
	testPutImageAligned();
	testPutImageUnaligned();
	testPutImageMask();
	testGetPutRoundTrip();
	testRawGetAlignedRoundTrip();
	testCopies();
	testKeyedCopy();
	testReveals();
	testCursorShapes();
	testCursorDrawing();
	testDispatch();
TEST_MAIN_END()

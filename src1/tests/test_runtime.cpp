/* Yendor1::Shared::Runtime: input polling, string drawing, delays, tile drawing and the record file API. */

#include "TestUtil.h"
#include "shared/Runtime.h"

#include <deque>
#include <stdio.h>
#include <string.h>

using namespace Yendor1;
using Shared::byte;

class ScriptedSystem : public Shared::System {
public:
	std::deque<int> keys;
	uint32_t now;
	int pumps;
	ScriptedSystem() : now(0), pumps(0) {}
	bool pump() { pumps++; return true; }
	void flushKeys() { keys.clear(); }
	int pollKey() { if (keys.empty()) return -1; const int k = keys.front(); keys.pop_front(); return k; }
	Shared::uint32 ticks() { return now++; }       // every read advances the clock by one tick
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
};

static void testPollInput() {
	Vid::Video v;
	v.init(true);
	ScriptedSystem sys;
	Shared::Runtime rt(v, sys);

	CHECK_EQ(rt.pollInput().source, Shared::InputEvent::None);
	sys.keys.push_back('a');
	Shared::InputEvent e = rt.pollInput();
	CHECK_EQ(e.source, Shared::InputEvent::Key);
	CHECK_EQ(e.code, 'A');                         // letters are upper-cased
	CHECK_EQ(v.ctx.textChar, 'A');
	sys.keys.push_back('5');
	CHECK_EQ(rt.pollInput().code, '5');
	sys.keys.push_back(0);
	sys.keys.push_back(0x48);                      // up arrow
	e = rt.pollInput();
	CHECK_EQ(e.source, Shared::InputEvent::Extended);
	CHECK_EQ(e.code, 0x48);
	CHECK(sys.keys.empty());

	// mouse: the driver's flags become Enter and are consumed
	v.ctx.mouseFlags = 0x60;
	e = rt.pollInput();
	CHECK_EQ(e.source, Shared::InputEvent::LeftClick);
	CHECK_EQ(e.code, 0x0D);
	CHECK_EQ(v.ctx.mouseFlags, 0x00);
	v.ctx.mouseFlags = 0x42;
	e = rt.pollInput();
	CHECK_EQ(e.source, Shared::InputEvent::RightClick);
	CHECK_EQ(v.ctx.mouseFlags, 0x00);
	// no mouse: flags are ignored
	Vid::Video v2;
	v2.init(false);
	Shared::Runtime rt2(v2, sys);
	v2.ctx.mouseFlags = 0x60;
	CHECK_EQ(rt2.pollInput().source, Shared::InputEvent::None);
	// a pending movement triggers the cursor refresh, which clears bit 7 when the cursor is hidden
	v.ctx.mouseFlags = 0x80;
	rt.pollInput();
	CHECK_EQ(v.ctx.mouseFlags & 0x80, 0);

	// waitInput blocks (pumping) until something arrives
	sys.pumps = 0;
	sys.keys.push_back('x');
	CHECK_EQ(rt.waitInput().code, 'X');
	CHECK(sys.pumps >= 1);
}

static void testDrawString() {
	static byte font[256 * 14];
	memset(font, 0, sizeof(font));
	for (int c = 0; c < 256; c++)
		font[c * 14 + 3] = (byte)c;                // row 3 of each glyph is the character code as a bit pattern
	Vid::Video v;
	v.setFont8x14(font);
	v.init(true);
	Shared::Runtime rt(v, *(new ScriptedSystem));
	v.ctx.textCol = 2; v.ctx.textRow = 1;
	v.ctx.textFg = 15; v.ctx.textBg = 4;
	rt.drawString("\xFF\x80");
	// x = 16..31, y = 14 + 3: first glyph all set, second only its top bit
	CHECK_EQ(v.pixel(0, 16, 17), 15);
	CHECK_EQ(v.pixel(0, 23, 17), 15);
	CHECK_EQ(v.pixel(0, 24, 17), 15);
	CHECK_EQ(v.pixel(0, 25, 17), 4);
	CHECK_EQ(v.pixel(0, 16, 16), 4);               // background fills the rest of the cell
	CHECK_EQ(v.ctx.textCol, 2);                    // the cell position is restored
	CHECK_EQ(v.ctx.textRow, 1);
	// the cursor show counter is balanced by the hide/show pair
	CHECK_EQ(v.ctx.cursorHide, -1);

	const char *lines[3] = {"\xFF", "\xFF", "\xFF"};
	v.ctx.textCol = 0; v.ctx.textRow = 4;
	rt.drawStringLines(lines, 3);
	CHECK_EQ(v.pixel(0, 0, 4 * 14 + 3), 15);
	CHECK_EQ(v.pixel(0, 0, 5 * 14 + 3), 15);
	CHECK_EQ(v.pixel(0, 0, 6 * 14 + 3), 15);
	CHECK_EQ(v.pixel(0, 0, 7 * 14 + 3), 0);
	CHECK_EQ(v.ctx.textRow, 7);
}

static void testDelayAndTiles() {
	Vid::Video v;
	v.init(false);
	ScriptedSystem sys;
	Shared::Runtime rt(v, sys);
	rt.delayTicks(5);
	CHECK(sys.now >= 5);

	// a two-tile "file"
	byte raw[1024];
	memset(raw, 0, sizeof(raw));
	raw[0] = 0x80;                                 // tile 0: plane 3 row 0 leftmost pixel
	raw[512 + 3 * 128 + 31 * 4 + 3] = 0x01;        // tile 1: plane 0 last row rightmost pixel
	rt.tiles[3].assign(raw, sizeof(raw));
	v.ctx.x1 = 64; v.ctx.y1 = 32;
	CHECK(rt.drawTile(3, 0));
	CHECK_EQ(v.pixel(0, 64, 32), 8);
	CHECK(!rt.drawTile(3, 2));
	CHECK(!rt.drawTile(9, 0));
	v.ctx.x1 = 0; v.ctx.y1 = 0;
	rt.drawTileRun(3, 0, 2);
	CHECK_EQ(v.ctx.x1, 64);                        // advanced by 2 tiles
	CHECK_EQ(v.pixel(0, 0, 0), 8);
	CHECK_EQ(v.pixel(0, 63, 31), 1);
}

static void testRecordFile() {
	const char *path = "test_runtime_records.tmp";
	{
		FILE *f = fopen(path, "wb");
		byte rec[8];
		for (int i = 0; i < 5; i++) { memset(rec, i + 1, 8); fwrite(rec, 1, 8, f); }
		fclose(f);
	}
	Shared::RecordFile rf;
	CHECK(rf.open(path, 8, true));
	CHECK_EQ(rf.recordCount(), 5u);
	byte buf[8];
	CHECK(rf.read(3, buf));
	CHECK_EQ(buf[0], 4); CHECK_EQ(buf[7], 4);
	memset(buf, 0xAA, 8);
	CHECK(rf.write(1, buf));
	byte again[8];
	CHECK(rf.read(1, again));
	CHECK_EQ(again[5], 0xAA);
	CHECK(!rf.read(5, again));                     // past the end: a short read fails
	rf.close();
	CHECK(!rf.read(0, again));
	Shared::RecordFile missing;
	CHECK(!missing.open("no_such_file.dat", 8, false));
	remove(path);
}

TEST_MAIN_BEGIN("shared runtime")
	testPollInput();
	testDrawString();
	testDelayAndTiles();
	testRecordFile();
TEST_MAIN_END()

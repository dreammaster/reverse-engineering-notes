#include "prog2/Prog2.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog2 {

namespace {

// PROG2.EXE's data segment starts 1870h bytes into its load image. The five story pages are consecutive NUL-terminated lines
// (64 bytes apart) at these data-segment offsets, and the animation entries live in the table from DS:1641h.
const uint32_t kDataBase = 0x1870;
const uint16 kTableStart = 0x1641;
const uint16 kTableBytes = 0x130;
const uint16 kPageText[5] = {0x0A41, 0x0CC1, 0x0F41, 0x11C1, 0x1441};
const int kPageLines[5] = {10, 10, 10, 10, 8};

// the six chains of animation entries: DS:1641h holds their first entries (word_12EB1 .. word_12EBB)
const uint16 kChainThird1 = 0x1641, kChainThird2 = 0x1643, kChainKettle = 0x1645, kChainFourth = 0x1647;
const uint16 kChainFifth1 = 0x1649, kChainFifth2 = 0x164B;

// fields of one animation entry
enum { kTile = 0x00, kX = 0x02, kY = 0x04, kCols = 0x06, kRows = 0x08, kDelay = 0x0A, kCounter = 0x0C, kNext = 0x0E, kRandom = 0x10 };

} // namespace

Prog2::Prog2(Shared::Runtime &rt, const Shared::Params &params)
	: _rt(rt), _c(rt.video.ctx), _ui(rt), _params(params), _haveExe(false), _tileIndex(0), _inputKeys(0), _state(0), _cursorHidden(0),
	  _curlOffset(0), _curlLeft(1), _curlRight(0x1F), _curlFrame(0), _quit(false) {
	std::string path = rt.dataDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "PROG2.EXE";
	_haveExe = _exe.load(path.c_str(), kDataBase);
	_table.assign(kTableBytes, 0);
	if (_haveExe) {
		const byte *src = _exe.bytesAt(kTableStart, kTableBytes);
		if (src)
			memcpy(&_table[0], src, kTableBytes);
	}
}

// ---------------------------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------------------------

uint16 Prog2::word(uint16 dsOffset) const {
	const size_t i = (size_t)(dsOffset - kTableStart);
	if (i + 1 >= _table.size())
		return 0;
	return (uint16)(_table[i] | (_table[i + 1] << 8));
}

void Prog2::setWord(uint16 dsOffset, uint16 value) {
	const size_t i = (size_t)(dsOffset - kTableStart);
	if (i + 1 >= _table.size())
		return;
	_table[i] = (byte)value;
	_table[i + 1] = (byte)(value >> 8);
}

void Prog2::draw() {
	_rt.drawTile(0, _tileIndex);
}

void Prog2::drawRun(int count) {
	for (int i = 0; i < count; i++) {
		draw();
		_c.x1 = (uint16)(_c.x1 + 0x20);
		_tileIndex++;
	}
}

void Prog2::tile(uint16 x, uint16 y, uint16 index) {
	_c.x1 = x;
	_c.y1 = y;
	_tileIndex = index;
	draw();
}

// the page text: `lines` consecutive strings from the exe, drawn from text row `row`, column 7, in the page colours
void Prog2::drawText(uint16 textOffset, int lines, byte row) {
	_c.textFg = _c.colourMap[0];
	_c.textBg = _c.colourMap[15];
	_c.textRow = row;
	_c.textCol = 7;
	std::vector<const char *> strings;
	uint32_t off = textOffset;
	for (int i = 0; i < lines; i++) {
		const char *s = _haveExe ? _exe.stringAt(off) : 0;
		strings.push_back(s ? s : "");
		off += s ? (uint32_t)strlen(s) + 1 : 0;
	}
	_rt.drawStringLines(&strings[0], lines);
}

bool Prog2::skipped() const {
	return _quit || (_params.parm(3) == 1 && _c.textChar == 0x1B);
}

// ---------------------------------------------------------------------------------------------
// sub_10A7B: the delays in the animation table are loop counts; they are multiplied up on the faster CPU classes
// ---------------------------------------------------------------------------------------------

void Prog2::scaleDelays() {
	if (_c.cpuClass == 0x88)
		return;
	const uint16 factor = _c.cpuClass == 0x286 ? 5 : 10;
	for (uint16 list = kTableStart; word(list) != 0; list += 2) {
		uint16 e = word(list);
		for (;;) {
			const uint16 delay = (uint16)(word(e + kDelay) * factor);
			setWord(e + kDelay, delay);
			setWord(e + kCounter, delay);
			if (word(e + kRandom) != 0)
				setWord(e + kRandom, (uint16)(word(e + kRandom) * factor));
			if ((int16_t)word(e + kNext) < (int16_t)e)      // the last entry points back to the first
				break;
			e = word(e + kNext);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// sub_102FC: count one entry down and, when it expires, draw its next picture (cols x rows tiles) and pick the next delay
// ---------------------------------------------------------------------------------------------

uint16 Prog2::stepAnimation(uint16 e) {
	uint16 counter = (uint16)(word(e + kCounter) - 1);
	setWord(e + kCounter, counter);
	if (counter != 0)
		return e;
	if (word(e + kRandom) != 0) {
		uint16 v;
		do {
			v = _ui.random(word(e + kRandom));
		} while ((int16_t)v < (int16_t)word(e + kDelay));
		setWord(e + kCounter, v);
	} else {
		setWord(e + kCounter, word(e + kDelay));
	}
	_tileIndex = word(e + kTile);
	_c.y1 = word(e + kY);
	for (uint16 row = 0; row < word(e + kRows); row++) {
		_c.x1 = word(e + kX);
		for (uint16 col = 0; col < word(e + kCols); col++) {
			// hide the pointer while a tile is drawn under it
			bool hidden = false;
			if (!((int16_t)(_c.curX + 0x10) < (int16_t)_c.x1 || (int16_t)(_c.x1 + 0x20) < (int16_t)_c.curX ||
			      (int16_t)(_c.curY + 0x10) < (int16_t)_c.y1 || (int16_t)(_c.y1 + 0x20) < (int16_t)_c.curY)) {
				hidden = true;
				_rt.vid(0x29);
			}
			draw();
			if (hidden)
				_rt.vid(0x28);
			_tileIndex++;
			_c.x1 = (uint16)(_c.x1 + 0x20);
		}
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	return word(e + kNext);
}

// ---------------------------------------------------------------------------------------------
// sub_105E4: hide the pointer when it is over the rectangle (x1,y1)-(x2,y2) that is about to be drawn
// ---------------------------------------------------------------------------------------------

void Prog2::hideCursorIfOver() {
	_cursorHidden = 0;
	if ((int16_t)(_c.curX + 0x10) < (int16_t)_c.x1 || (int16_t)_c.x2 < (int16_t)_c.curX ||
	    (int16_t)(_c.curY + 0x10) < (int16_t)_c.y1 || (int16_t)_c.y2 < (int16_t)_c.curY)
		return;
	_rt.vid(0x29);
	_cursorHidden = 1;
}

// sub_10625: blank the text area of the book page
void Prog2::clearTextArea() {
	_c.x1 = 0x30;
	_c.y1 = 0x0E;
	_c.x2 = 0x24F;
	_c.y2 = 0x12D;
	_c.colour = _c.colourMap[15];
	_rt.vid(0x07);
}

// ---------------------------------------------------------------------------------------------
// the five pages
// ---------------------------------------------------------------------------------------------

void Prog2::drawPage1() {
	clearTextArea();
	_c.x1 = 0x80; _c.y1 = 0x11; _tileIndex = 0xF0; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0x31; _tileIndex = 0x104; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0x51; _tileIndex = 0x118; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0x71; _tileIndex = 0x12C; drawRun(12);
	_c.colour = _c.colourMap[7];
	_c.x1 = 0x80; _c.x2 = 0x200; _c.y1 = 0x11; _c.y2 = 0x90;
	_rt.vid(0x06);
	drawText(kPageText[0], kPageLines[0], 0x0B);
	tile(0x50, 0x94, 0xDC);
	_ui.drawTopCorners();
}

void Prog2::drawPage2() {
	_rt.vid(0x29);
	clearTextArea();
	_c.x1 = 0x80; _c.y1 = 0xAA; _tileIndex = 0x140; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0xCA; _tileIndex = 0x14C; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0xEA; _tileIndex = 0x158; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0x10A; _tileIndex = 0x164; drawRun(12);
	_ui.drawBottomCorners();
	drawText(kPageText[1], kPageLines[1], 2);
	tile(0x50, 0x16, 0xE1);
}

void Prog2::drawPage3() {
	_rt.vid(0x29);
	clearTextArea();
	drawText(kPageText[2], kPageLines[2], 0x0B);
	tile(0x50, 0x94, 0xC9);
	_c.x1 = 0x60; _c.y1 = 0x11; _tileIndex = 0x1A0; drawRun(14);
	_c.x1 = 0x60; _c.y1 = 0x31; _tileIndex = 0x1AE; drawRun(14);
	_c.x1 = 0x60; _c.y1 = 0x51; _tileIndex = 0x1BC; drawRun(14);
	_c.x1 = 0x60; _c.y1 = 0x71; _tileIndex = 0x1CA; drawRun(14);
	_ui.drawTopCorners();
}

void Prog2::drawPage4() {
	_rt.vid(0x29);
	clearTextArea();
	drawText(kPageText[3], kPageLines[3], 2);
	_c.x1 = 0xA0; _c.y1 = 0xBA; _tileIndex = 0x222; drawRun(4);
	_c.x1 = 0x120; _c.y1 = 0xBA; _tileIndex = 0x22C; drawRun(6);
	_c.x1 = 0xA0; _c.y1 = 0xDA; _tileIndex = 0x232; drawRun(10);
	_c.x1 = 0xA0; _c.y1 = 0xFA; _tileIndex = 0x23C; drawRun(10);
	_ui.drawBottomCorners();
	tile(0x50, 0x16, 0xE2);
}

void Prog2::drawPage5() {
	_rt.vid(0x29);
	clearTextArea();
	drawText(kPageText[4], kPageLines[4], 0x0D);
	tile(0x50, 0xB0, 0xC8);
	_c.x1 = 0xA0; _c.y1 = 0x11; _tileIndex = 0x248; drawRun(10);
	_c.x1 = 0xA0; _c.y1 = 0x31; _tileIndex = 0x252; drawRun(10);
	_c.x1 = 0xA0; _c.y1 = 0x51; _tileIndex = 0x25C; drawRun(10);
	_c.x1 = 0xA0; _c.y1 = 0x71; _tileIndex = 0x266; drawRun(10);
	_c.x1 = 0xA0; _c.y1 = 0x91; _tileIndex = 0x270; drawRun(10);
	_ui.drawTopCorners();
}

// ---------------------------------------------------------------------------------------------
// waiting
// ---------------------------------------------------------------------------------------------

// shared_21818f with BL=0: wait for a key or the icon
byte Prog2::blockingWait() {
	_state = _ui.input((byte)(_inputKeys >> 8), (byte)_inputKeys, 0);
	if (_state == 0xFF) {
		_quit = true;
		_state = 1;
	}
	return _state;
}

// sub_11750: poll for input without blocking until it comes or `seconds` have passed (state 2)
byte Prog2::waitWithTimeout(int seconds) {
	const byte keys = (byte)(_inputKeys >> 8), icons = (byte)_inputKeys;
	_state = _ui.input(keys, icons, 0x80);
	if (_state == 0xFF) { _quit = true; _state = 1; return _state; }
	if (_state != 2)
		return _state;
	const uint32 start = _rt.system.ticks();
	const uint32 span = (uint32)seconds * 91 / 5;           // 18.2 BIOS ticks per second
	for (;;) {
		_state = _ui.input(keys, icons, 0x80);
		if (_state == 0xFF) { _quit = true; _state = 1; return _state; }
		if (_state != 2)
			return _state;
		if ((uint32)(_rt.system.ticks() - start) >= span) {
			_state = 2;
			return _state;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// page 1: the page curl (sub_103AF, sub_10AD5)
// ---------------------------------------------------------------------------------------------

// sub_10AD5: one frame of the curl on page 1, with the strips beside it blanked and the page edges drawn
void Prog2::curlFrame() {
	_c.drawPage = 1;
	const uint16 x0 = (uint16)(0x80 - _curlOffset);
	_c.x1 = x0; _c.y1 = 0x11; _tileIndex = (uint16)(0xF0 + _curlFrame); drawRun(13);
	_c.x1 = x0; _c.y1 = 0x31; _tileIndex = (uint16)(0x104 + _curlFrame); drawRun(13);
	_c.x1 = x0; _c.y1 = 0x51; _tileIndex = (uint16)(0x118 + _curlFrame); drawRun(13);
	_c.x1 = x0; _c.y1 = 0x71; _tileIndex = (uint16)(0x12C + _curlFrame); drawRun(13);
	_c.y1 = 0x11;
	_c.y2 = 0x90;
	_c.colour = _c.colourMap[15];
	if (_curlLeft != 0) {
		_c.x2 = 0x7F;
		_c.x1 = (uint16)(0x7F - _curlLeft);
		_rt.vid(0x07);
	}
	if (_curlRight != 0) {
		_c.x1 = 0x201;
		_c.x2 = (uint16)(0x201 + _curlRight);
		_rt.vid(0x07);
	}
	_c.colour = _c.colourMap[7];
	_c.x1 = 0x80; _c.x2 = 0x80;
	_rt.vid(0x05);
	_c.x1 = 0x200; _c.x2 = 0x200;
	_rt.vid(0x05);
}

void Prog2::pageCurl() {
	_rt.vid(0x29);
	for (int group = 0; group < 8; group++) {
		_curlOffset = 1;
		_curlLeft = 1;
		_curlRight = 0x20;
		for (int step = 0; step < 16; step++) {
			curlFrame();
			_rt.vid(0x12, 1);
			_c.x1 = 0x80; _c.y1 = 0x11; _c.x2 = 0x200; _c.y2 = 0x91; _c.x3 = 0x80; _c.y3 = 0x11;
			_rt.vid(0x23);
			_curlLeft += 2;
			_curlRight -= 2;
			_curlOffset += 2;
		}
		_curlFrame++;
	}
	_c.drawPage = 0;
	_rt.vid(0x28);
	blockingWait();
}

void Prog2::firstPageWait() {
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	waitWithTimeout(20);
	if (_state == 2 && !_quit)
		pageCurl();
}

// ---------------------------------------------------------------------------------------------
// page 2: the picture appears pixel by pixel (sub_1045A, sub_11392)
// ---------------------------------------------------------------------------------------------

// sub_11392: copy the 12 x 4 blocks' worth of page 1 onto page 0, one pixel position at a time in random order. The area is cut into
// 64 x 64 cells; every pass copies the same random pixel of each of the 6 x 2 cells.
void Prog2::pixelDissolve() {
	const byte srcPage = 1, dstPage = 0;
	const uint16 cellRows = 4 / 2, cellCols = 0x0C / 2;
	const uint16 srcX = 0x80, srcY = 0xAA, dstX = 0x80, dstY = 0xAA;
	static const byte bitMasks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	byte visited[512];
	memset(visited, 0, sizeof(visited));
	uint16 remaining = 0x1000;
	uint16 retry = 0xFDA6;
	uint32 misses = 0;

	// shared_33aaab: true when the position was already taken, otherwise marks it
	struct Taken {
		byte *v;
		bool operator()(uint16 ox, uint16 oy) const {
			const uint16 bit = (uint16)((oy << 6) + ox);
			const byte m = bitMasks[bit & 7];
			byte &b = v[bit >> 3];
			if (b & m)
				return true;
			b |= m;
			return false;
		}
	} taken = {visited};

	while (remaining != 0) {
		uint16 ox = 0, oy = 0;
		// shared_1f8a83
		if ((int16_t)remaining > 5) {
			for (;;) {
				ox = _ui.random(0x3F);
				oy = _ui.random(0x3F);
				if (!taken(ox, oy)) {
					misses = 0;
					break;
				}
				// every 602nd miss the original clears the generator's seed so the next draw is seeded from the clock again (this is
				// what gets it out of the generator's short cycles); the second reseed is not in the original (a clock that stands still)
				if (++retry == 0) {
					retry = 0xFDA6;
					_ui.rng.setSeed(0);
				}
				if (++misses > 100000) {
					misses = 0;
					_ui.rng.setSeed(0);
				}
			}
		} else {
			for (;;) {
				if (ox == 0x3F) {
					if (oy == 0x3F)
						break;
					ox = 0;
					oy++;
				} else {
					ox++;
				}
				if (!taken(ox, oy))
					break;
			}
		}
		uint16 cellY = 0;
		for (uint16 r = 0; r < cellRows; r++) {
			uint16 cellX = 0;
			for (uint16 q = 0; q < cellCols; q++) {
				_c.x1 = (uint16)(srcX + cellX + ox);
				_c.y1 = (uint16)(srcY + cellY + oy);
				_c.drawPage = srcPage;
				_rt.vid(0x04);
				_c.x1 = (uint16)(dstX + cellX + ox);
				_c.y1 = (uint16)(dstY + cellY + oy);
				_c.drawPage = dstPage;
				_rt.vid(0x03);
				cellX = (uint16)(cellX + 0x40);
			}
			cellY = (uint16)(cellY + 0x40);
		}
		remaining--;
	}
}

void Prog2::secondPage() {
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	waitWithTimeout(5);
	if (_state != 2 || _quit)
		return;
	_rt.vid(0x29);
	_c.drawPage = 1;
	_c.x1 = 0x80; _c.y1 = 0xAA; _tileIndex = 0x170; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0xCA; _tileIndex = 0x17C; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0xEA; _tileIndex = 0x188; drawRun(12);
	_c.x1 = 0x80; _c.y1 = 0x10A; _tileIndex = 0x194; drawRun(12);
	_c.drawPage = 0;
	pixelDissolve();
	_rt.vid(0x28);
	blockingWait();
}

// ---------------------------------------------------------------------------------------------
// pages 3 to 5: pictures that keep moving until the icon is clicked
// ---------------------------------------------------------------------------------------------

// sub_10BD2: the occasional kettle/cauldron animation on page 3
void Prog2::kettleAnimation() {
	_c.x1 = 0xC0; _c.y1 = 0x51; _c.x2 = 0x140; _c.y2 = 0x91;
	hideCursorIfOver();
	_c.x1 = 0xC0; _c.y1 = 0x71; _tileIndex = 0x1E9; drawRun(2);
	_tileIndex = 0x22A; drawRun(2);
	_rt.delayTicks(2);
	_c.x1 = 0xC0; _c.y1 = 0x71; _tileIndex = 0x1CD; drawRun(2);
	_c.x1 = 0x120; _c.y1 = 0x51; _tileIndex = 0x1E6; draw();
	_c.y1 = (uint16)(_c.y1 + 0x20); _c.x1 = (uint16)(_c.x1 - 0x20); _tileIndex++; drawRun(2);
	_rt.delayTicks(2);
	_c.x1 = 0x100; _c.y1 = 0x71; _tileIndex = 0x1CF; draw();
	_c.y1 = (uint16)(_c.y1 - 0x20); _c.x1 = (uint16)(_c.x1 + 0x20); _tileIndex = 0x1C2; draw();
	_c.x1 = 0x140; _c.y1 = 0x51; _tileIndex = 0x1E3; draw();
	_c.y1 = (uint16)(_c.y1 + 0x20); _c.x1 = (uint16)(_c.x1 - 0x20); _tileIndex++; drawRun(2);
	_rt.delayTicks(2);
	_c.x1 = 0x140; _c.y1 = 0x51; _tileIndex = 0x1C3; draw();
	_c.y1 = (uint16)(_c.y1 + 0x20); _c.x1 = (uint16)(_c.x1 - 0x20); _tileIndex = 0x1D0; drawRun(2);
	if (_cursorHidden == 1)
		_rt.vid(0x28);
}

void Prog2::thirdPageLoop() {
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	do {
		setWord(kChainThird1, stepAnimation(word(kChainThird1)));
		setWord(kChainThird2, stepAnimation(word(kChainThird2)));
		const uint16 k = word(kChainKettle);
		const uint16 counter = (uint16)(word(k + kCounter) - 1);
		setWord(k + kCounter, counter);
		if (counter == 0) {
			setWord(k + kCounter, word(k + kDelay));
			kettleAnimation();
		}
		_state = _ui.input((byte)(_inputKeys >> 8), (byte)_inputKeys, 0x80);
	} while (_state == 2);
	if (_state == 0xFF)
		_quit = true;
}

void Prog2::fourthPageLoop() {
	do {
		setWord(kChainFourth, stepAnimation(word(kChainFourth)));
		_state = _ui.input((byte)(_inputKeys >> 8), (byte)_inputKeys, 0x80);
	} while (_state == 2);
	if (_state == 0xFF)
		_quit = true;
}

void Prog2::fifthPageLoop() {
	do {
		setWord(kChainFifth1, stepAnimation(word(kChainFifth1)));
		setWord(kChainFifth2, stepAnimation(word(kChainFifth2)));
		_state = _ui.input((byte)(_inputKeys >> 8), (byte)_inputKeys, 0x80);
	} while (_state == 2);
	if (_state == 0xFF)
		_quit = true;
}

// ---------------------------------------------------------------------------------------------
// the program
// ---------------------------------------------------------------------------------------------

int Prog2::run() {
	scaleDelays();
	// (the original preloads the tiles of every page here; the tile cache of this port loads them on first use)
	_rt.sendPaletteCommand(0x0A);
	_rt.sendPaletteCommand(3);
	_rt.system.flushKeys();                  // shared_5c3cce
	_rt.vid(0x29);
	// shared_d2992b reads the ten party records into the shared buffer; this program never uses them.
	_rt.vid(0x14);
	_inputKeys = 0x0404;
	if (_params.parm(3) == 1)
		_inputKeys = 0x4444;                 // Esc skips the prologue
	_state = 0;
	_ui.drawIconBar((byte)_inputKeys);
	drawPage1();
	_rt.sendPaletteCommand(7);
	_rt.vid(0x16);
	// (shared_728316 plays a sound cue here when a sound card is present; none is supported)
	firstPageWait();
	if (skipped()) goto leave;

	_rt.vid(0x14);
	drawPage2();
	_rt.vid(0x16);
	secondPage();
	if (_c.videoKind != 2) {
		_rt.sendPaletteCommand(0x0A);
		_rt.sendPaletteCommand(3);
	}
	if (skipped()) goto leave;

	_rt.vid(0x14);
	drawPage3();
	_rt.vid(0x16);
	thirdPageLoop();
	if (skipped()) goto leave;

	_rt.vid(0x14);
	drawPage4();
	_rt.vid(0x16);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	fourthPageLoop();
	if (skipped()) goto leave;

	_rt.vid(0x14);
	drawPage5();
	_rt.vid(0x16);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	fifthPageLoop();
	_state = 0;
	if (_quit)
		_state = 1;
	goto done;

leave:
	_state = 1;
done:
	_rt.vid(0x29);
	return _state;
}

} // namespace Prog2
} // namespace Yendor1

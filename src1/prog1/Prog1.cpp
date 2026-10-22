#include "prog1/Prog1.h"

#include <stddef.h>
#include <string.h>

namespace Yendor1 {
namespace Prog1 {

namespace {

// PROG1.EXE's data segment starts 1F50h bytes into its load image; the copyright line is at DS:0E99h and the 17
// credit lines of the "About the Authors" page are consecutive strings from DS:0EB7h
const uint32_t kDataBase = 0x1F50;
const uint32_t kCopyrightOffset = 0x0E99;
const uint32_t kAuthorsOffset = 0x0EB7;

} // namespace

Prog1::Prog1(Shared::Runtime &rt, const Shared::Params &params)
	: _rt(rt), _c(rt.video.ctx), _ui(rt), _params(params), _tileFile(0), _exitCode(0), _copyright("") {
	std::string path = rt.dataDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "PROG1.EXE";
	const bool ok = _exe.load(path.c_str(), kDataBase);
	uint32_t off = kAuthorsOffset;
	for (int i = 0; i < 17; i++) {
		const char *s = ok ? _exe.stringAt(off) : 0;
		_authorLines[i] = s ? s : "";
		off += s ? (uint32_t)strlen(s) + 1 : 0;
	}
	if (ok && _exe.stringAt(kCopyrightOffset))
		_copyright = _exe.stringAt(kCopyrightOffset);
}

// ---------------------------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------------------------

void Prog1::tile(uint16 x, uint16 y, uint16 index) {
	_c.x1 = x;
	_c.y1 = y;
	_rt.drawTile(_tileFile, index);
}

void Prog1::run(uint16 firstIndex, int count) {
	_rt.drawTileRun(_tileFile, firstIndex, count);
}

void Prog1::setColours(byte fg, byte bg) {
	_c.textFg = fg;
	_c.textBg = bg;
}

// ---------------------------------------------------------------------------------------------
// the routines, in address order
// ---------------------------------------------------------------------------------------------

// sub_102E4: the copyright line on a cleared text screen, with a short speaker beep
void Prog1::showCopyrightLine() {
	_rt.sendPaletteCommand(5);
	_rt.system.speaker((uint16)(1193180 / 0x578));
	_c.textBg = colour(0);
	_c.textFg = colour(3);
	if (_c.videoKind != 1)
		_c.textFg = colour(11);
	_c.textCol = 0x17;
	_c.textRow = 0x18;
	_rt.drawString(_copyright);
	_rt.system.speaker(0);
	_rt.sendPaletteCommand(1);
}

// sub_1033A: fourteen horizontal lines below the picture, one per tick
void Prog1::drawRuleLines() {
	_c.colour = colour(0);
	_c.x1 = 0; _c.y1 = 0x150; _c.x2 = 0x27F; _c.y2 = 0x150;
	for (int i = 0; i < 0x0E; i++) {
		_rt.vid(0x05);
		_c.y1++;
		_c.y2++;
		_rt.delayTicks(1);
	}
}

// sub_1037E: fill the 20 x 11 tile area of page 1 with the background tiles from 362h
void Prog1::fillBackgroundTiles() {
	if (_c.videoKind != 2)
		_c.drawPage = 1;
	_c.y1 = 0;
	_tileFile = 0;
	uint16 index = 0x362;
	for (int row = 0; row < 0x0B; row++) {
		_c.x1 = 0;
		for (int col = 0; col < 0x14; col++) {
			_rt.drawTile(_tileFile, index++);
			_c.x1 = (uint16)(_c.x1 + 0x20);
		}
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	_c.drawPage = 0;
}

// sub_103D2: the "About the Authors" page (the open book with the credit lines), drawn on page 1
void Prog1::drawAuthorsPage() {
	_c.drawPage = 1;
	_c.x1 = 0x30; _c.y1 = 0x0E; _c.x2 = 0x24F; _c.y2 = 0x12D;
	_c.colour = colour(15);
	_rt.vid(0x07);
	_c.textFg = colour(0);
	_ui.drawTopCorners();
	_ui.drawBottomCorners();
	_c.textRow = 3;
	_c.textCol = 0x0B;
	setColours(colour(0), colour(15));
	_rt.drawStringLines(_authorLines, 17);
	_tileFile = 0;
	tile(0xE8, 0x20, 0xDC);                // the illuminated first letter
}

// sub_1046D: the title picture grows in with row copies between the pages
void Prog1::titleSweeps() {
	_c.y1 = 0x20;
	_c.x1 = 0;
	_c.drawPage = 1;
	_rt.drawTile(_tileFile, 0x71);
	_c.y1 = 0;
	_rt.drawTile(_tileFile, 0x67);
	_c.x1 = (uint16)(_c.x1 + 0x20);
	run(0x35, 7);
	_c.y1 = 0; _c.x1 = 0;
	_c.y3 = 0x6F; _c.x3 = 0x108;
	_c.destPage = 0;
	for (int i = 0; i < 0x20; i++) {
		_rt.vid(0x0D, 1);
		_c.y1++; _c.y3++;
		_rt.delayTicks(1);
	}
	_c.y1 = 0x20;
	_c.y3 = 0x8F;
	_c.x3 = 0x108;
	for (int i = 0; i < 0x20; i++) {
		_rt.vid(0x0D, 1);
		_c.y1++; _c.y3++;
		_rt.delayTicks(1);
	}
	_c.y1 = 0; _c.x1 = 0x20;
	_c.y3 = 0xD5; _c.x3 = 0xDF;
	for (int i = 0; i < 0x20; i++) {
		_rt.vid(0x0D, 6);
		_c.y1++; _c.y3++;
		_rt.delayTicks(1);
	}
	_c.drawPage = 0;
}

// sub_1056C: dissolve the lower title area from page 1 onto page 0 (black pixels are not copied)
void Prog1::dissolveTitle() {
	Shared::DissolveParams d;
	d.srcPage = 1;
	d.dstPage = 0;
	d.blockRows = 6;
	d.blockCols = 0x14;
	d.srcX = 0; d.srcY = 0x40;
	d.dstX = 0; d.dstY = 0x40;
	d.skipColour = colour(0);
	_ui.dissolve(d);
}

// sub_105A7: black panel plus two tile blocks on page 1
void Prog1::drawLowerPanels() {
	_c.drawPage = 1;
	_c.x1 = 0x32; _c.x2 = 0xFA; _c.y1 = 0x91; _c.y2 = 0xAF;
	_c.colour = colour(0);
	_rt.vid(0x07);
	_c.y1 = 0x40;
	uint16 index = 0x43E;
	for (int row = 0; row < 3; row++) {
		_c.x1 = 0;
		_rt.drawTileRun(_tileFile, index, 0x14);
		index = (uint16)(index + 0x14);
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	for (int row = 0; row < 3; row++) {
		_c.x1 = 0xC0;
		_rt.drawTileRun(_tileFile, index, 8);
		index = (uint16)(index + 8);
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	_c.drawPage = 0;
}

// sub_1061D: a frame shrinking to black (65 nested rectangles)
void Prog1::shrinkingFrame() {
	if (_c.cpuClass != 0x88)
		_rt.delayTicks(0x28);
	if (_c.videoKind == 2)
		return;                         // the SVGA path fades the palette instead (not part of this install)
	_c.x1 = 0x95; _c.y1 = 0x6F; _c.x2 = 0x1D0; _c.y2 = 0xF5;
	_c.colour = colour(0);
	for (int i = 0; i < 0x41; i++) {
		_rt.vid(0x06);
		_c.x1++; _c.y1++; _c.x2--; _c.y2--;
		_rt.video.busyWait(_c.delayCpu);
	}
	_rt.sendPaletteCommand(5);
	_rt.sendPaletteCommand(3);
}

// sub_106C8: page 1 gets the book background; on the first run the twelve mouse cursor shapes are captured from tiles
void Prog1::drawBookBackground() {
	_c.drawPage = 1;
	if (_params.parm(2) != 1) {
		_c.mousePresent = 0;
		_c.mouseFlags = 0;
	} else {
		_c.x1 = 0; _c.y1 = 0;
		_tileFile = 5;
		_rt.drawTileRun(_tileFile, 0x10F, 3);
		_c.y2 = 0x0F;
		int slot = 1;
		for (int row = 0; row < 2; row++) {
			_c.x1 = 0;
			for (int col = 0; col < 6; col++) {
				_c.x2 = (uint16)(_c.x1 + 0x0F);
				_c.image.set(_rt.video.cursorStore() + slot * Vid::kCursorSlotBytes, Vid::kCursorSlotBytes, 0);
				_rt.vid(0x0B);
				slot++;
				_c.x1 = (uint16)(_c.x1 + 0x10);
			}
			_c.y1 = (uint16)(_c.y1 + 0x10);
			_c.y2 = (uint16)(_c.y2 + 0x10);
		}
	}
	_tileFile = 0;
	_c.x1 = 0; _c.y1 = 0; _c.x2 = 0x27F; _c.y2 = 0x15D;
	if (_c.videoKind == 2)
		_c.y2 = 0x18F;
	_c.colour = colour(15);
	_rt.vid(0x07);
	_rt.drawTile(_tileFile, 0xD5);
	_c.x1 = 0x20;
	run(0xE5, 2);
	_c.x1 = (uint16)(_c.x1);
	run(0xE9, 6);
	run(0xDA, 2);
	run(0x1F4, 8);
	_c.x1 = 0x260;
	_rt.drawTile(_tileFile, 0xD3);
	_c.y1 = 0x20;
	const int rows = _c.videoKind == 2 ? 0x0B : 9;
	for (int i = 0; i < rows; i++) {
		tile(0, _c.y1, 0xD6);
		tile(0x20, _c.y1, 0xEF);
		tile(0x240, _c.y1, 0x1FD);
		_c.x1 = (uint16)(_c.x1 + 0x20);
		_rt.drawTile(_tileFile, 0xD4);
		_c.y1 = (uint16)(_c.y1 + 0x20);
	}
	_c.x1 = 0;
	_c.y1 = 0x13F;
	if (_c.videoKind != 2) {
		_rt.drawTile(_tileFile, 0xD7);
		tile(0x20, _c.y1, 0x1FC);
		tile(0x240, _c.y1, 0x1FE);
		tile(0x260, _c.y1, 0xD2);
		_c.y1 = (uint16)(_c.y1 - 0x20);
		tile(0x120, _c.y1, 0x200);
		tile(0x140, _c.y1, 0x201);
		_c.y1 = (uint16)(_c.y1 + 0x20);
		tile(0x120, _c.y1, 0xD8);
		tile(0x140, _c.y1, 0xD9);
		_c.x1 = 0x2F; _c.y1 = 0x12E;
		run(0x202, 8);
		_c.x1 = 0x2F;
		_c.y1 = (uint16)(_c.y1 + 0x20);
		run(0x20A, 8);
		_c.x1 = 0x38; _c.x2 = 0x55; _c.y1 = 0x135; _c.y2 = 0x139;
	} else {
		// SVGA layout: not part of this install (no VID256.DRV)
		return;
	}
	_c.colour = colour(2);
	_rt.vid(0x07);
	_ui.drawBottomDecorations();
	_c.x1 = 0x2F; _c.y1 = 0x0D; _c.x2 = 0x250; _c.y2 = 0x12E;
	_c.colour = colour(7);
	_rt.vid(0x06);
	_c.drawPage = 0;
}

// sub_10B2C: touches tiles 0..76h and A0h..D1h of FILE00 so the cache has them loaded; there is no cache here
void Prog1::preloadTiles() {
	_tileFile = 0;
}

// sub_10B60: curtains: rows copied outward from the middle of page 1 to page 0
void Prog1::closeCurtains() {
	if (_c.cpuClass != 0x88)
		_rt.delayTicks(0x19);
	if (_c.videoKind == 2)
		return;
	_c.drawPage = 1;
	_c.destPage = 0;
	_c.x1 = 0;
	_c.x3 = 0;
	uint16 up = 0xA9, down = 0xAA;
	for (int i = 0; i < 0xAA; i++) {
		_c.y1 = up; _c.y3 = up;
		_rt.vid(0x0D, 0x14);
		_c.y1 = down; _c.y3 = down;
		_rt.vid(0x0D, 0x14);
		up--;
		down++;
		_rt.video.busyWait(_c.delayCpu);
	}
	_c.drawPage = 0;
}

// sub_10BE6: poll once; Esc ends the intro with exit code 1
bool Prog1::escapePressed() {
	_c.textChar = 0;
	const Shared::InputEvent ev = _rt.pollInput();
	if (ev.source != Shared::InputEvent::Key)
		return false;
	if (_c.textChar != 0x1B)
		return false;
	_exitCode = 1;
	_rt.sendPaletteCommand(5);
	return true;
}

// sub_10C2C: the logo animation: frames of a figure assembled from FILE00 tiles, with ticks between frames
void Prog1::logoAnimation() {
	_c.x1 = 0x128; _c.y1 = 0x7F;
	_tileFile = 0;
	if (_c.cpuClass != 0x88) {
		for (int i = 0; i < 3; i++)
			_rt.drawTile(_tileFile, (uint16)(3 + i));        // three frames at one spot, drawn on top of each other
		_c.x1 = 0x108; run(6, 3);
		_c.x1 = 0x108; run(0, 3);
		_c.y1 = 0x5F; _c.x1 = 0x108; run(0x14, 2);
		_c.y1 = 0x7F; _c.x1 = 0xE8; run(0x0E, 4);
		_c.y1 = 0x9F; _c.x1 = 0x128; run(0x18, 2);
		_c.y1 = 0x9F; _c.x1 = 0x128;
		_rt.drawTile(_tileFile, 0x3B); _c.x1 = (uint16)(_c.x1 + 0x20);
		_rt.drawTile(_tileFile, 0x3B); _c.x1 = (uint16)(_c.x1 + 0x20);
		_c.y1 = 0x5F; _c.x1 = 0x108;
		_rt.drawTile(_tileFile, 0x3B); _c.x1 = (uint16)(_c.x1 + 0x20);
		_rt.drawTile(_tileFile, 0x3B); _c.x1 = (uint16)(_c.x1 + 0x20);
		_c.y1 = 0x7F; _c.x1 = 0xC8; run(0x12, 2);
		run(9, 5);                                           // continues at the x where the previous run stopped
		_c.x1 = 0xA8; run(0x16, 2);
		_c.x1 = 0xE8; run(0x1A, 7);
		tile(0x1A8, _c.y1, 0x3B);
		_c.x1 = 0xA8; run(0x21, 8);
		_c.x1 = 0xA8; run(0x2B, 9);
	}
	_c.y1 = 0x6F; _c.x1 = 0xA8; run(0x64, 10);
	_c.y1 = 0x8F; _c.x1 = 0xA8; run(0x6E, 10);

	for (int pass = 0; pass < 6; pass++) {
		for (int half = 0; half < 2; half++) {
			// frame A: the figure's three-tile rows at y 6Fh and 8Fh
			tile(0xE8, 0x6F, 0x66); tile(0x108, 0x6F, 0x3C); tile(0x128, 0x6F, 0x68);
			tile(0xE8, 0x8F, 0x70); tile(0x108, 0x8F, 0x46); tile(0x128, 0x8F, 0x72);
			_rt.delayTicks(1);
			// frames B, C, D: three-tile rows 3Dh/47h, 40h/4Ah, 43h/4Dh
			static const uint16 rows[3][2] = {{0x3D, 0x47}, {0x40, 0x4A}, {0x43, 0x4D}};
			for (int f = 0; f < 3; f++) {
				_c.y1 = 0x6F; _c.x1 = 0xE8; run(rows[f][0], 3);
				_c.y1 = 0x8F; _c.x1 = 0xE8; run(rows[f][1], 3);
				_rt.delayTicks(1);
			}
		}
	}
	// final frame A, no pause
	tile(0xE8, 0x6F, 0x66); tile(0x108, 0x6F, 0x3C); tile(0x128, 0x6F, 0x68);
	tile(0xE8, 0x8F, 0x70); tile(0x108, 0x8F, 0x46); tile(0x128, 0x8F, 0x72);
	drawRuleLines();
	_rt.delayTicks(0x23);
}

// ---------------------------------------------------------------------------------------------
// the program
// ---------------------------------------------------------------------------------------------

void Prog1::delayAfterSequence(bool paletteHelper) {
	uint16 ticks = 0x0A;
	if (_c.cpuClass != 0x88)
		ticks = (uint16)(ticks + 0x46);
	_rt.delayTicks(ticks);
	if (paletteHelper) {
		_rt.sendPaletteCommand(0x0A);
		_rt.sendPaletteCommand(5);
	}
}

int Prog1::run() {
	// shared_d2992b reads the ten party records into the shared buffer; this program never uses them.
	_rt.system.flushKeys();                  // shared_5c3cce
	_tileFile = 0;
	_exitCode = 0;
	switch (_params.parm(3)) {
	case 0:
		// the whole first-run sequence, ending on the authors page
		showCopyrightLine();
		preloadTiles();
		logoAnimation();
		titleSweeps();
		shrinkingFrame();
		fillBackgroundTiles();
		closeCurtains();
		drawLowerPanels();
		dissolveTitle();
		drawBookBackground();
		drawAuthorsPage();
		delayAfterSequence(false);
		_ui.drawIconBar(4);
		_rt.vid(0x1F);                       // reveal page 1 from the centre outward onto page 0
		_exitCode = 0;
		_c.drawPage = 0;
		_rt.vid(0x12, 0);
		_rt.vid(0x28);
		_ui.waitForInput(4);
		break;
	case 1: {
		// the same sequence, abortable with Esc after every step
		bool aborted = false;
		showCopyrightLine();
		aborted = escapePressed();
		if (!aborted) { preloadTiles(); aborted = escapePressed(); }
		if (!aborted) { logoAnimation(); aborted = escapePressed(); }
		if (!aborted) { titleSweeps(); aborted = escapePressed(); }
		if (!aborted) { shrinkingFrame(); aborted = escapePressed(); }
		if (!aborted) { fillBackgroundTiles(); aborted = escapePressed(); }
		if (!aborted) { closeCurtains(); aborted = escapePressed(); }
		if (!aborted) { drawLowerPanels(); aborted = escapePressed(); }
		if (!aborted) { dissolveTitle(); aborted = escapePressed(); }
		if (!aborted)
			delayAfterSequence(_c.videoKind != 2);
		drawBookBackground();
		_rt.vid(0x1E);
		break;
	}
	case 2:
		// from the book menu: only the authors page
		_rt.vid(0x29);
		drawAuthorsPage();
		_ui.drawIconBar(4);
		_c.drawPage = 0;
		_rt.vid(0x20);
		_rt.vid(0x12, 0);
		_rt.vid(0x28);
		_ui.waitForInput(4);
		break;
	case 3:
		_rt.sendPaletteCommand(5);
		drawBookBackground();
		_rt.vid(0x1E);
		_exitCode = 0;
		break;
	default:
		_exitCode = 0xFF;
		break;
	}
	return _exitCode;
}

} // namespace Prog1
} // namespace Yendor1

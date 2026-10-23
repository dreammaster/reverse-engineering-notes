#include "shared/Ui.h"


namespace Yendor1 {
namespace Shared {

namespace {

const IconEntry kIcons[8] = {
	{0x80, 0x20, 0x157, 0x15A, 0x174, 0x3, 0x1D, 0x213, 0x214},
	{0x40, 0x1B, 0x178, 0x17E, 0x194, 0x6, 0x19, 0x217, 0x218},
	{0x20, 0x49, 0x196, 0x19C, 0x1B2, 0x6, 0x19, 0x20B, 0x20C},
	{0x10, 0x51, 0x1B4, 0x1BA, 0x1D0, 0x6, 0x19, 0x20D, 0x20E},
	{0x08, 0x4B, 0x1D2, 0x1D8, 0x1EE, 0x6, 0x19, 0x20F, 0x210},
	{0x04, 0x4D, 0x1F0, 0x1EC, 0x20C, 0x6, 0x18, 0x211, 0x212},
	{0x02, 0x0D, 0x20E, 0x214, 0x22A, 0x6, 0x19, 0x219, 0x21A},
	{0x01, 0x20, 0x22C, 0x232, 0x248, 0xA, 0x19, 0x215, 0x216},
};

const byte kBitMasks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

} // namespace

// ---------------------------------------------------------------------------------------------
// shared_9c11a6
// ---------------------------------------------------------------------------------------------

uint16 Rng::next(uint16 max, uint16 clockSeed) {
	uint16 ax = _seed;
	if (ax == 0)
		ax = clockSeed;
	ax = (uint16)(0 - ax);                              // neg ax
	const uint32 product = (uint32)ax * 0x805;          // mul bx
	ax = (uint16)product;
	const uint16 dx = (uint16)(product >> 16);
	ax = (uint16)((ax >> 8) | (ax << 8));               // xchg ah, al
	_seed = ax;
	if ((int16)(dx - _seed) >= 0)                       // sub dx, seed; js skips
		ax = (uint16)(ax + 1);                          // inc ax / inc ax / add ax, FFFFh
	int cx = 0;
	uint16 m = max;
	for (;;) {                                           // count the leading zero bits of max
		const bool carry = m & 0x8000;
		m = (uint16)(m << 1);
		if (carry)
			break;
		cx++;
		if (cx > 16)
			break;
	}
	ax = (uint16)(ax << cx);
	ax = (uint16)(ax >> cx);                            // keep only the bits max can use
	if (ax > max)
		ax = (uint16)(ax - max);
	return ax;
}

// ---------------------------------------------------------------------------------------------
// BookUi
// ---------------------------------------------------------------------------------------------

BookUi::BookUi(Runtime &rt) : _rt(rt) {
}

uint16 BookUi::random(uint16 max) {
	return rng.next(max, rng.seed() == 0 ? _rt.system.dosClock() : 0);
}

void BookUi::drawIconTile(uint16 tile) {
	_rt.drawTile(5, tile);
}

void BookUi::drawTopCorners() {
	Vid::Context &c = _rt.video.ctx;
	c.x1 = 0x30; c.y1 = 0x0E;
	_rt.drawTile(0, 0xDD);
	c.y1 = 0x2E;
	_rt.drawTile(0, 0xE7);
	c.x1 = 0x230; c.y1 = 0x0E;
	_rt.drawTile(0, 0xDE);
	c.y1 = 0x2E;
	_rt.drawTile(0, 0xE8);
}

void BookUi::drawBottomCorners() {
	Vid::Context &c = _rt.video.ctx;
	c.x1 = 0x30; c.y1 = 0x10E;
	_rt.drawTile(0, 0xDF);
	c.x1 = 0x230;
	_rt.drawTile(0, 0xE0);
}

void BookUi::drawBottomDecorations() {
	Vid::Context &c = _rt.video.ctx;
	if (c.videoKind == 2) {
		c.y1 = 0x131;
		c.x1 = 0x151;
		_rt.drawTileRun(0, 0x2A8, 8);
		c.y1 = (uint16)(c.y1 + 0x20); c.x1 = 0x151;
		_rt.drawTileRun(0, 0x2B2, 8);
		c.y1 = (uint16)(c.y1 + 0x20); c.x1 = 0x151;
		_rt.drawTileRun(0, 0x2BC, 8);
		c.iconBarState = 0;
		return;
	}
	c.x1 = 0x151; c.y1 = 0x12E;
	uint16 tile = 0x212;
	for (int i = 0; i < 8; i++) {
		_rt.drawTile(0, tile++);
		c.x1 = (uint16)(c.x1 + 0x20);
	}
	c.x1 = 0x151;
	c.y1 = (uint16)(c.y1 + 0x20);
	for (int i = 0; i < 8; i++) {
		_rt.drawTile(0, tile++);
		c.x1 = (uint16)(c.x1 + 0x20);
	}
	c.iconBarState = 0;
}

void BookUi::drawIconBar(byte mask) {
	Vid::Context &c = _rt.video.ctx;
	if (c.mousePresent == 0)
		return;
	c.cursorShape = 1;
	c.iconBarState = 2;
	c.x1 = 0x154; c.x2 = 0x24D; c.y1 = 0x132; c.y2 = 0x15A;
	c.colour = c.colourMap[15];
	if (c.videoKind == 2) {
		c.y1 = 0x135; c.y2 = 0x179;
		_rt.vid(0x07);
		c.y1 = 0x148;
	} else {
		_rt.vid(0x07);
		c.y1 = 0x135;
	}
	for (int i = 0; i < 8; i++) {
		if (kIcons[i].flags & mask) {
			c.x1 = kIcons[i].x;
			drawIconTile(kIcons[i].tileUp);
		}
	}
}

bool BookUi::iconClick(byte mask) {
	Vid::Context &c = _rt.video.ctx;
	c.y1 = c.videoKind == 2 ? 0x148 : 0x135;
	int i = 0;
	for (;; i++) {
		if (i == 8) {
			_rt.last.source = InputEvent::None;
			return false;
		}
		const int ax = (int16)c.curX;
		if (ax < (int16)kIcons[i].hitXMin)
			{ _rt.last.source = InputEvent::None; return false; }
		if (ax <= (int16)kIcons[i].hitXMax)
			break;
	}
	const IconEntry &e = kIcons[i];
	const int ay = (int16)c.curY;
	int bx = (int16)(c.y1 + e.hitYMin);
	if (ay < bx) { _rt.last.source = InputEvent::None; return false; }
	bx = (int16)(c.y1 + e.hitYMax);
	if (ay > bx) { _rt.last.source = InputEvent::None; return false; }
	if (!(e.flags & mask)) { _rt.last.source = InputEvent::None; return false; }
	_rt.vid(0x29);
	c.x1 = e.x;
	drawIconTile(e.tileDown);
	_rt.vid(0x28);
	_rt.delayTicks(5);
	_rt.vid(0x29);
	drawIconTile(e.tileUp);
	_rt.vid(0x28);
	_rt.last.source = InputEvent::Key;
	_rt.last.code = e.key;
	c.textChar = e.key;
	return true;
}

void BookUi::waitForInput(byte mask) {
	Vid::Context &c = _rt.video.ctx;
	c.y1 = c.videoKind == 2 ? 0x148 : 0x135;
	for (;;) {
		if (!_rt.system.pump())
			return;
		const InputEvent ev = _rt.pollInput();
		if (ev.source == InputEvent::None) {
			_rt.system.idle();
			continue;                       // the BL flag (bit 7) is never set by the callers in these programs
		}
		if (ev.source == InputEvent::LeftClick) {
			if (iconClick(mask))
				return;
			continue;                       // a click that misses every icon is ignored (BL & 20h is 0)
		}
		if (ev.source == InputEvent::RightClick) {
			if (iconClick(mask))
				return;
			continue;
		}
		// any key ends the wait
		_rt.last.source = InputEvent::None;
		return;
	}
}

byte BookUi::input(byte keyMask, byte clickMask, byte flags) {
	Vid::Context &c = _rt.video.ctx;
	c.y1 = c.videoKind == 2 ? 0x148 : 0x135;
	for (;;) {
		if (!_rt.system.pump())
			return 0xFF;
		const InputEvent ev = _rt.pollInput();
		if (ev.source == InputEvent::None) {
			if (flags & 0x80)
				return 2;
			_rt.system.idle();
			continue;
		}
		if (ev.source == InputEvent::LeftClick || ev.source == InputEvent::RightClick) {
			if (iconClick(clickMask))
				return 1;
			if (flags & 0x20) {
				if (ev.source == InputEvent::RightClick)
					c.textChar = 0;
				return ev.source == InputEvent::LeftClick ? 3 : 4;
			}
			continue;
		}
		// a key: with no mask any key counts, else it has to be the key of an icon enabled for the mask
		if (keyMask == 0)
			return 0;
		for (int i = 0; i < 8; i++) {
			if (c.textChar == kIcons[i].key && (kIcons[i].flags & keyMask))
				return 0;
		}
	}
}

void BookUi::drawBox(const byte *r) {
	Vid::Context &c = _rt.video.ctx;
	auto word = [r](int i) { return (uint16)(r[i] | (r[i + 1] << 8)); };
	const uint16 off = r[9];
	c.x1 = (uint16)(word(0) + off);
	c.y1 = (uint16)(word(2) + off);
	c.x2 = (uint16)(word(4) + off);
	c.y2 = (uint16)(word(6) + off);
	c.colour = r[8];
	_rt.vid(0x07);                                   // the shadow
	c.x1 = (uint16)(c.x1 - off);
	c.y1 = (uint16)(c.y1 - off);
	c.x2 = (uint16)(c.x2 - off);
	c.y2 = (uint16)(c.y2 - off);
	c.colour = c.colourMap[7];
	_rt.vid(0x06);                                   // the frame
	c.x1++; c.y1++; c.x2--; c.y2--;
	c.colour = c.colourMap[15];
	_rt.vid(0x07);                                   // the inside
}

void BookUi::openConfirmPanel(const byte *boxRecord, const char *title1, const char *title2, const char *label, byte iconMask) {
	Vid::Context &c = _rt.video.ctx;
	if (_panelSaved.empty())
		_panelSaved.assign(1000 * 16, 0);            // bx = 3E8h paragraphs
	_rt.vid(0x29);
	c.x1 = 0x197; c.y1 = 0xE9; c.x2 = 0x24F; c.y2 = 0x12D;
	const Vid::ImagePtr saved = c.image;
	c.image.set(&_panelSaved[0], (uint32_t)_panelSaved.size(), 0);
	_rt.vid(0x0B);                                   // vidGetImage into the buffer
	c.image = saved;
	_rt.vid(0x14);
	drawBox(boxRecord);
	c.fontSelect = 0;
	c.textCol = 0x34;
	c.textRow = 0x11;
	c.textBg = c.colourMap[15];
	c.textFg = c.colourMap[0];
	_rt.drawString(title1);
	c.textRow = 0x12;
	_rt.drawString(title2);
	c.textFg = c.colourMap[11];
	c.textCol = 0x34;
	c.textRow = 0x13;
	_rt.drawString(label);
	drawIconBar(iconMask);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
}

void BookUi::closeInfoPanel() {
	Vid::Context &c = _rt.video.ctx;
	_rt.vid(0x29);
	_rt.vid(0x14);
	c.x1 = 0x197; c.y1 = 0xE9; c.x2 = 0x24F; c.y2 = 0x12D;
	if (!_panelSaved.empty()) {
		const Vid::ImagePtr saved = c.image;
		c.image.set(&_panelSaved[0], (uint32_t)_panelSaved.size(), 0);
		_rt.vid(0x0C);                               // vidPutImageHdr from the buffer
		c.image = saved;
	}
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
}

void BookUi::refreshAndShow() {
	drawBottomDecorations();
	_rt.video.ctx.cursorShape = _rt.video.ctx.cursorShape;   // (no change; kept for symmetry with the original's flow)
	_rt.vid(0x12, 0);
}

// ---------------------------------------------------------------------------------------------
// the dissolve (shared_27c37f, shared_1f8a83, shared_33aaab, shared_47fa83)
// ---------------------------------------------------------------------------------------------

void BookUi::dissolve(const DissolveParams &p) {
	Vid::Context &c = _rt.video.ctx;
	byte visited[128];
	for (int i = 0; i < 128; i++)
		visited[i] = 0;
	uint16 remaining = 0x400;
	uint16 counter = 0xFDA6;
	uint32 misses = 0;

	// shared_33aaab: returns true when the offset pair was already taken, otherwise marks it
	auto taken = [&](uint16 ox, uint16 oy) {
		const uint16 bit = (uint16)((oy << 5) + ox);
		const byte m = kBitMasks[bit & 7];
		byte &b = visited[bit >> 3];
		if (b & m)
			return true;
		b |= m;
		return false;
	};

	while (remaining != 0) {
		uint16 ox = 0, oy = 0;
		// shared_1f8a83
		if (remaining > 5) {
			for (;;) {
				ox = random(0x1F);
				oy = random(0x1F);
				if (!taken(ox, oy))
					break;
				// every 602nd miss the original clears the generator's seed, so the next draw is seeded from the clock again;
				// that is what gets it out of the generator's short cycles
				if (++counter == 0) {
					counter = 0xFDA6;
					rng.setSeed(0);
				}
				// a clock that never changes would still trap it: give up on the cycle after many misses (not in the original)
				if (++misses > 100000) {
					misses = 0;
					rng.setSeed(0);
				}
			}
		} else {
			ox = 0; oy = 0;
			for (;;) {
				if (ox == 0x1F) {
					if (oy == 0x1F)
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
		for (uint16 row = 0; row < p.blockRows; row++) {
			for (uint16 col = 0; col < p.blockCols; col++) {
				c.x1 = (uint16)(p.srcX + col * 0x20 + ox);
				c.y1 = (uint16)(p.srcY + row * 0x20 + oy);
				c.drawPage = p.srcPage;
				_rt.vid(0x04);
				if (c.colour != p.skipColour) {
					c.x1 = (uint16)(p.dstX + col * 0x20 + ox);
					c.y1 = (uint16)(p.dstY + row * 0x20 + oy);
					c.drawPage = p.dstPage;
					_rt.vid(0x03);
				}
			}
		}
		remaining--;
	}
	c.drawPage = 0;
}

} // namespace Shared
} // namespace Yendor1

#include "vid/Video.h"

#include <string.h>

#include "data/Palette.h"

namespace Yendor1 {
namespace Vid {

namespace {

inline uint16 rol16(uint16 v, int s) {
	s &= 15;
	return s ? (uint16)((v << s) | (v >> (16 - s))) : v;
}

inline int16 s16(uint16 v) {
	return (int16)v;
}

} // namespace

Context::Context()
	: tileSlots(200), delayStep(0), delayWipe(0), delayCpu(0), cpuClass(0),
	  imgRows(0x20), imgBytes(4), imgMask(0xFF),
	  x1(0), y1(0), x2(0), y2(0), x3(0), y3(0),
	  textCol(0), textRow(0), textChar(0), textFg(0), textBg(0),
	  colour(0), drawPage(0), destPage(0), videoKind(1), keyColour(0), fontSelect(0),
	  paletteCommand(0), viewHalfW(0), viewHalfH(0), hotMarginX(0), hotMarginY(0), viewOffX(0), viewOffY(0),
	  flags912(0), mousePresent(0), mouseFlags(0), cursorHide(-1), cursorShape(1), cursorPage(0), cursorMode(0),
	  clipX1(0), clipX2(0x27F), clipY1(0), clipY2(0x15D), curX(0), curY(0), mouseX(0), mouseY(0), hotX(0x130), hotY(0x8E) {
}

Video::Video()
	: _font14(0), _font8(0), _host(0), _displayed(0), _scratchRows(0) {
	memset(_vram, 0, sizeof(_vram));
	memset(_cursorStore, 0, sizeof(_cursorStore));
	for (int i = 0; i < 16; i++)
		_palette[i] = (byte)i;
}

void Video::setPaletteRegisters(const byte regs[16]) {
	memcpy(_palette, regs, 16);
}

void Video::loadPlane(int p, const byte *data) {
	memcpy(_vram[p], data, kPlaneBytes);
}

namespace {

inline uint16 g16(const byte *b, int o) { return (uint16)(b[o] | (b[o + 1] << 8)); }
inline void p16(byte *b, int o, uint16 v) { b[o] = (byte)v; b[o + 1] = (byte)(v >> 8); }

} // namespace

void Video::loadContext(const byte *b) {
	ctx.tileSlots = g16(b, 0x00);
	ctx.delayStep = g16(b, 0x08); ctx.delayWipe = g16(b, 0x0A); ctx.delayCpu = g16(b, 0x0C); ctx.cpuClass = g16(b, 0x0E);
	ctx.imgRows = g16(b, 0x10); ctx.imgBytes = g16(b, 0x12); ctx.imgMask = b[0x14];
	ctx.x1 = g16(b, 0x1A); ctx.y1 = g16(b, 0x1C); ctx.x2 = g16(b, 0x1E); ctx.y2 = g16(b, 0x20);
	ctx.x3 = g16(b, 0x22); ctx.y3 = g16(b, 0x24);
	ctx.textCol = b[0x2A]; ctx.textRow = b[0x2B]; ctx.textChar = b[0x2C]; ctx.textFg = b[0x2E]; ctx.textBg = b[0x2F];
	ctx.colour = b[0x30]; ctx.drawPage = b[0x31]; ctx.destPage = b[0x32]; ctx.videoKind = b[0x33];
	ctx.keyColour = b[0x34]; ctx.fontSelect = b[0x35];
	ctx.paletteCommand = b[0x676];
	ctx.viewHalfW = b[0x678]; ctx.viewHalfH = b[0x679];
	ctx.hotMarginX = g16(b, 0x67A); ctx.hotMarginY = g16(b, 0x67C); ctx.viewOffX = g16(b, 0x67E); ctx.viewOffY = g16(b, 0x680);
	ctx.flags912 = b[0x912];
	ctx.mousePresent = b[0x952]; ctx.mouseFlags = b[0x953]; ctx.cursorHide = (int8_t)b[0x955];
	ctx.cursorShape = b[0x956]; ctx.cursorPage = b[0x957]; ctx.cursorMode = b[0x958];
	ctx.clipX1 = (int16)g16(b, 0x95A); ctx.clipX2 = (int16)g16(b, 0x95C); ctx.clipY1 = (int16)g16(b, 0x95E); ctx.clipY2 = (int16)g16(b, 0x960);
	ctx.curX = g16(b, 0x962); ctx.curY = g16(b, 0x964);
	ctx.mouseX = (int16)g16(b, 0x966); ctx.mouseY = (int16)g16(b, 0x968);
	ctx.hotX = g16(b, 0x96A); ctx.hotY = g16(b, 0x96C);
}

void Video::storeContext(byte *b) const {
	const Context &c = ctx;
	p16(b, 0x00, c.tileSlots);
	p16(b, 0x08, c.delayStep); p16(b, 0x0A, c.delayWipe); p16(b, 0x0C, c.delayCpu); p16(b, 0x0E, c.cpuClass);
	p16(b, 0x10, c.imgRows); p16(b, 0x12, c.imgBytes); b[0x14] = c.imgMask;
	p16(b, 0x1A, c.x1); p16(b, 0x1C, c.y1); p16(b, 0x1E, c.x2); p16(b, 0x20, c.y2); p16(b, 0x22, c.x3); p16(b, 0x24, c.y3);
	b[0x2A] = c.textCol; b[0x2B] = c.textRow; b[0x2C] = c.textChar; b[0x2E] = c.textFg; b[0x2F] = c.textBg;
	b[0x30] = c.colour; b[0x31] = c.drawPage; b[0x32] = c.destPage; b[0x33] = c.videoKind; b[0x34] = c.keyColour; b[0x35] = c.fontSelect;
	b[0x676] = c.paletteCommand;
	b[0x678] = c.viewHalfW; b[0x679] = c.viewHalfH;
	p16(b, 0x67A, c.hotMarginX); p16(b, 0x67C, c.hotMarginY); p16(b, 0x67E, c.viewOffX); p16(b, 0x680, c.viewOffY);
	b[0x912] = c.flags912;
	b[0x952] = c.mousePresent; b[0x953] = c.mouseFlags; b[0x955] = (byte)c.cursorHide;
	b[0x956] = c.cursorShape; b[0x957] = c.cursorPage; b[0x958] = c.cursorMode;
	p16(b, 0x95A, (uint16)c.clipX1); p16(b, 0x95C, (uint16)c.clipX2); p16(b, 0x95E, (uint16)c.clipY1); p16(b, 0x960, (uint16)c.clipY2);
	p16(b, 0x962, c.curX); p16(b, 0x964, c.curY); p16(b, 0x966, (uint16)c.mouseX); p16(b, 0x968, (uint16)c.mouseY);
	p16(b, 0x96A, c.hotX); p16(b, 0x96C, c.hotY);
}

void Video::delay(uint16 n) {
	if (_host)
		_host->delay(n);
}

// ----------------------------------------------------------------------------
// addressing and raw pixel access (sub_112C0, sub_1142D, sub_10DA2)
// ----------------------------------------------------------------------------

Video::Addr Video::calcAddr(int x, int y) const {
	Addr a;
	uint16 off = (uint16)((uint16)y * kRowBytes);
	off = (uint16)(off + ((uint16)x >> 3));
	if (ctx.drawPage != 0)
		off = (uint16)(off + 0x8000);
	a.offset = off;
	a.shift = (x & 7) ^ 7;
	return a;
}

void Video::storeMasked(int plane, uint16 offset, byte mask, byte value) {
	_vram[plane][offset] = (byte)((_vram[plane][offset] & ~mask) | (value & mask));
}

void Video::putPixelAt(const Addr &a, byte colour) {
	const byte mask = (byte)(1 << a.shift);
	for (int p = 0; p < 4; p++)
		storeMasked(p, a.offset, mask, (colour >> p) & 1 ? 0xFF : 0x00);
}

byte Video::getPixelAt(const Addr &a) const {
	const byte mask = (byte)(1 << a.shift);
	byte c = 0;
	for (int p = 0; p < 4; p++)
		if (_vram[p][a.offset] & mask)
			c |= (byte)(1 << p);
	return c;
}

byte Video::pixel(int page, int x, int y) const {
	const uint16 off = (uint16)((page ? 0x8000 : 0) + y * kRowBytes + (x >> 3));
	const byte mask = (byte)(0x80 >> (x & 7));
	byte c = 0;
	for (int p = 0; p < 4; p++)
		if (_vram[p][off] & mask)
			c |= (byte)(1 << p);
	return c;
}

void Video::renderIndexed(int page, byte *out) const {
	for (int y = 0; y < kScreenHeight; y++)
		for (int x = 0; x < kScreenWidth; x++)
			out[y * kScreenWidth + x] = pixel(page, x, y);
}

void Video::renderRgb(int page, byte *out) const {
	Data::Rgb pal[16];
	for (int i = 0; i < 16; i++)
		pal[i] = Data::egaToRgb(_palette[i]);
	for (int y = 0; y < kScreenHeight; y++) {
		for (int x = 0; x < kScreenWidth; x++) {
			const Data::Rgb &c = pal[pixel(page, x, y)];
			byte *d = out + (y * kScreenWidth + x) * 3;
			d[0] = c.r;
			d[1] = c.g;
			d[2] = c.b;
		}
	}
}

// ----------------------------------------------------------------------------
// 00-02 setup
// ----------------------------------------------------------------------------

void Video::init(bool mouse) {
	ctx.mousePresent = mouse ? 0xFF : 0;
	ctx.fontSelect = 0;
	ctx.videoKind = 1;
	ctx.cursorHide = -1;
	ctx.mouseFlags = 0;
	ctx.hotX = 0x130;
	ctx.hotY = 0x8E;
	for (int i = 0; i < 16; i++)
		_palette[i] = (byte)i;
}

void Video::shutdown() {
}

void Video::setGraphicsMode() {
	memset(_vram, 0, sizeof(_vram));
	_displayed = 0;
}

// ----------------------------------------------------------------------------
// 03-07 pixels, lines, rectangles
// ----------------------------------------------------------------------------

void Video::putPixel() {
	putPixelAt(calcAddr(ctx.x1, ctx.y1), ctx.colour);
}

void Video::getPixel() {
	ctx.colour = getPixelAt(calcAddr(ctx.x1, ctx.y1));
}

// sub_10E5D. Quirks reproduced: the endpoints are swapped in the context when x2 < x1; a vertical line
// always runs downward from (x1,y1) for |y2-y1|+1 rows even when y2 < y1.
void Video::lineImpl() {
	int cx = (int16)(ctx.x2 - ctx.x1);
	if (cx == 0) {
		int rows = (int16)(ctx.y2 - ctx.y1);
		if (rows < 0)
			rows = -rows;
		rows++;
		Addr a = calcAddr(ctx.x1, ctx.y1);
		for (int i = 0; i < rows; i++) {
			putPixelAt(a, ctx.colour);
			a.offset = (uint16)(a.offset + kRowBytes);
		}
		return;
	}
	if (cx < 0) {
		cx = -cx;
		uint16 t = ctx.x1; ctx.x1 = ctx.x2; ctx.x2 = t;
		t = ctx.y1; ctx.y1 = ctx.y2; ctx.y2 = t;
	}
	int dy = (int16)(ctx.y2 - ctx.y1);
	if (dy == 0) {
		for (int i = 0; i <= cx; i++)
			putPixelAt(calcAddr(ctx.x1 + i, ctx.y1), ctx.colour);
		return;
	}
	int step = kRowBytes;
	if (dy < 0) {
		dy = -dy;
		step = -kRowBytes;
	}
	Addr a = calcAddr(ctx.x1, ctx.y1);
	const bool xMajor = !(dy > cx);
	int major = xMajor ? cx : dy;
	int minor = xMajor ? dy : cx;
	int d = 2 * minor - major;
	const int dStay = 2 * minor;
	const int dMove = 2 * minor - 2 * major;
	int shift = a.shift;
	uint16 off = a.offset;
	for (int n = major + 1; n > 0; n--) {
		putPixelAt(Addr{off, shift}, ctx.colour);
		if (xMajor) {
			// x advances every step; y advances when the decision variable is non-negative
			if (--shift < 0) {
				shift = 7;
				off++;
			}
			if (d >= 0) {
				off = (uint16)(off + step);
				d += dMove;
			} else {
				d += dStay;
			}
		} else {
			// y advances every step; x advances when the decision variable is non-negative
			off = (uint16)(off + step);
			if (d >= 0) {
				if (--shift < 0) {
					shift = 7;
					off++;
				}
				d += dMove;
			} else {
				d += dStay;
			}
		}
	}
}

void Video::line() {
	lineImpl();
}

void Video::rect() {
	const uint16 sx1 = ctx.x1, sy1 = ctx.y1, sx2 = ctx.x2, sy2 = ctx.y2;
	ctx.y2 = ctx.y1;                 // top edge
	lineImpl();
	ctx.y1 = sy2;                    // bottom edge
	ctx.y2 = sy2;
	lineImpl();
	ctx.x2 = sx1;                    // left edge
	ctx.y1 = sy1;
	lineImpl();
	ctx.x1 = sx2;                    // right edge
	ctx.x2 = sx2;
	lineImpl();
	ctx.x1 = sx1;
	ctx.x2 = sx2;
	ctx.y1 = sy1;
	ctx.y2 = sy2;
}

void Video::fillRect() {
	const uint16 sx1 = ctx.x1, sy1 = ctx.y1, sx2 = ctx.x2, sy2 = ctx.y2;
	uint16 n = (uint16)(sy2 - sy1 + 1);   // a zero count loops 65536 times, as `loop` does
	ctx.y2 = ctx.y1;
	do {
		lineImpl();
		ctx.y1++;
		ctx.y2++;
	} while (--n != 0);
	ctx.x1 = sx1;
	ctx.x2 = sx2;
	ctx.y1 = sy1;
	ctx.y2 = sy2;
}

// ----------------------------------------------------------------------------
// 08 text
// ----------------------------------------------------------------------------

void Video::drawCharImpl() {
	const bool small = ctx.fontSelect != 0;
	const int height = small ? 8 : 14;
	const byte *font = small ? _font8 : _font14;
	const uint16 x = (uint16)(8 * ctx.textCol);
	const uint16 y = (uint16)(height * ctx.textRow);
	const Addr a = calcAddr(x, y);        // x is a multiple of 8: always the aligned path
	uint16 off = a.offset;
	for (int r = 0; r < height; r++) {
		const byte bits = font ? font[ctx.textChar * height + r] : 0;
		for (int p = 0; p < 4; p++) {
			const byte fg = (ctx.textFg >> p) & 1 ? 0xFF : 0x00;
			const byte bg = (ctx.textBg >> p) & 1 ? 0xFF : 0x00;
			_vram[p][off] = (byte)((bits & fg) | (~bits & bg));
		}
		off = (uint16)(off + kRowBytes);
	}
}

void Video::drawChar() {
	drawCharImpl();
}

// ----------------------------------------------------------------------------
// 09-0C images
// ----------------------------------------------------------------------------

namespace {

inline byte imgRead(const ImagePtr &p, uint32_t off) {
	return (p.data && off < p.size) ? p.data[off] : 0;
}

inline void imgWrite(ImagePtr &p, uint32_t off, byte v) {
	if (p.data && off < p.size)
		p.data[off] = v;
}

} // namespace

// sub_112EF: draw a planar image (planes 3,2,1,0, rows x width bytes each) with the right-edge mask.
void Video::putImageImpl() {
	const uint16 rows = ctx.imgRows;
	const uint16 W = ctx.imgBytes;
	const byte mask = ctx.imgMask;
	const Addr a = calcAddr(ctx.x1, ctx.y1);
	const int s = (a.shift + 1) & 7;
	uint32_t sp = ctx.image.offset;

	if (s == 0 && mask == 0xFF) {
		for (int plane = 3; plane >= 0; plane--) {
			uint16 rowOff = a.offset;
			for (uint16 r = 0; r < rows; r++) {
				for (uint16 c = 0; c < W; c++)
					_vram[plane][(uint16)(rowOff + c)] = imgRead(ctx.image, sp++);
				rowOff = (uint16)(rowOff + kRowBytes);
			}
		}
		return;
	}

	uint16 m16, b16;
	if (W == 1) {
		m16 = (uint16)(mask << 8);
		b16 = mask;
	} else {
		m16 = (uint16)(int16)(int8_t)mask;   // cbw
		b16 = 0x00FF;
	}
	m16 = (uint16)(m16 << s);
	b16 = (uint16)(b16 << s);
	const byte Mhi = (byte)(m16 >> 8), Mlo = (byte)m16, Bhi = (byte)(b16 >> 8);

	for (int plane = 3; plane >= 0; plane--) {
		uint16 rowOff = a.offset;
		for (uint16 r = 0; r < rows; r++) {
			uint16 di = rowOff;
			int bx = W;
			uint32_t rp = sp;
			uint16 V = (uint16)(imgRead(ctx.image, rp) | (imgRead(ctx.image, rp + 1) << 8));
			rp++;
			bool toFinal = false;
			if (s != 0) {
				V = rol16(V, s);
				storeMasked(plane, di, Bhi, (byte)(V >> 8));
				di++;
				bx--;
			} else {
				bx--;
				if (bx == 0)
					toFinal = true;
			}
			if (!toFinal) {
				bx--;
				if (bx > 0) {
					do {
						storeMasked(plane, di, 0xFF, (byte)V);
						di++;
						V = (uint16)(imgRead(ctx.image, rp) | (imgRead(ctx.image, rp + 1) << 8));
						rp++;
						V = rol16(V, s);
						bx--;
					} while (bx != 0);
				}
			}
			storeMasked(plane, di, Mhi, (byte)V);
			storeMasked(plane, (uint16)(di + 1), Mlo, (byte)(V >> 8));
			sp += W;
			rowOff = (uint16)(rowOff + kRowBytes);
		}
	}
}

void Video::putImage() {
	putImageImpl();
}

// sub_10DC1 (with header) and sub_10D23 (raw): capture a screen rectangle as planar data.
// Quirk reproduced: the raw variant shifts by (8-p)&7 instead of p, so it only round-trips for x % 8 == 0.
void Video::getImageImpl(bool withHeader) {
	uint16 rows, W;
	byte mask;
	int s;
	uint32_t dp = ctx.image.offset;
	Addr a = calcAddr(ctx.x1, ctx.y1);
	if (withHeader) {
		const uint16 dx = (uint16)(ctx.x2 - ctx.x1);
		mask = (byte)(0xFF << ((dx & 7) ^ 7));
		W = (uint16)((dx >> 3) + 1);
		rows = (uint16)(ctx.y2 - ctx.y1 + 1);
		s = a.shift ^ 7;                         // = x % 8
		imgWrite(ctx.image, dp++, (byte)rows);
		imgWrite(ctx.image, dp++, (byte)(rows >> 8));
		imgWrite(ctx.image, dp++, (byte)W);
		imgWrite(ctx.image, dp++, (byte)(W >> 8));
		imgWrite(ctx.image, dp++, mask);
	} else {
		rows = ctx.imgRows;
		W = ctx.imgBytes;
		mask = ctx.imgMask;
		s = (a.shift + 1) & 7;
	}
	for (int plane = 3; plane >= 0; plane--) {
		uint16 rowOff = a.offset;
		for (uint16 r = 0; r < rows; r++) {
			uint16 si = rowOff;
			uint32_t last = dp;
			for (uint16 c = 0; c < W; c++) {
				const uint16 V = (uint16)(_vram[plane][si] | (_vram[plane][(uint16)(si + 1)] << 8));
				si++;
				last = dp;
				imgWrite(ctx.image, dp++, (byte)rol16(V, s));
			}
			if (W)
				imgWrite(ctx.image, last, (byte)(imgRead(ctx.image, last) & mask));
			rowOff = (uint16)(rowOff + kRowBytes);
		}
	}
}

void Video::getImageRaw() {
	getImageImpl(false);
}

void Video::getImage() {
	getImageImpl(true);
}

// fn0C: read the 5-byte header (rows, width, mask) from the stream, advance past it, draw, reset the size fields.
void Video::putImageHdr() {
	uint32_t &off = ctx.image.offset;
	ctx.imgRows = (uint16)(imgRead(ctx.image, off) | (imgRead(ctx.image, off + 1) << 8));
	ctx.imgBytes = (uint16)(imgRead(ctx.image, off + 2) | (imgRead(ctx.image, off + 3) << 8));
	ctx.imgMask = imgRead(ctx.image, off + 4);
	off += 5;
	putImageImpl();
	ctx.imgRows = 0x20;
	ctx.imgBytes = 4;
	ctx.imgMask = 0xFF;
}

// ----------------------------------------------------------------------------
// 0D-11 VRAM-to-VRAM copies (write mode 1: all four planes move together)
// ----------------------------------------------------------------------------

void Video::copyBytes(uint16 dst, uint16 src, uint16 count, bool backward) {
	for (uint32_t i = 0; i < count; i++) {
		const uint16 d = backward ? (uint16)(dst - i) : (uint16)(dst + i);
		const uint16 s = backward ? (uint16)(src - i) : (uint16)(src + i);
		for (int p = 0; p < 4; p++)
			_vram[p][d] = _vram[p][s];
	}
}

void Video::copyTileRow(byte tiles) {
	uint16 si = (uint16)((ctx.x1 >> 3) + kRowBytes * ctx.y1);
	if (ctx.drawPage != 0)
		si = (uint16)(si + 0x8000);
	uint16 di = (uint16)((ctx.x3 >> 3) + kRowBytes * ctx.y3);
	if (ctx.destPage != 0)
		di = (uint16)(di + 0x8000);
	copyBytes(di, si, (uint16)(tiles * 4), false);
}

void Video::scrollRight() {
	uint16 si = (uint16)(kRowBytes * (uint16)(0x8E - ctx.viewOffY));
	si = (uint16)(si + (((uint16)(0x130 + ctx.viewOffX - 1)) >> 3));
	uint16 di = (uint16)(si + 4);
	uint16 rows = (uint16)(((byte)(ctx.viewHalfH * 2) + 1) << 5);
	uint16 cnt = (uint16)(((byte)(ctx.viewHalfW * 2)) << 2);
	do {
		copyBytes(di, si, cnt, true);
		di = (uint16)(di + kRowBytes);
		si = (uint16)(si + kRowBytes);
	} while (--rows != 0);
}

void Video::scrollLeft() {
	uint16 si = (uint16)(kRowBytes * (uint16)(0x8E - ctx.viewOffY));
	si = (uint16)(si + (((uint16)(0x130 - ctx.viewOffX + 0x20)) >> 3));
	uint16 di = (uint16)(si - 4);
	uint16 rows = (uint16)(((byte)(ctx.viewHalfH * 2) + 1) << 5);
	uint16 cnt = (uint16)(((byte)(ctx.viewHalfW * 2)) << 2);
	do {
		copyBytes(di, si, cnt, false);
		di = (uint16)(di + kRowBytes);
		si = (uint16)(si + kRowBytes);
	} while (--rows != 0);
}

void Video::scrollUp() {
	uint16 si = (uint16)(kRowBytes * (uint16)(0x8E - ctx.viewOffY));
	si = (uint16)(si + (((uint16)(0x130 - ctx.viewOffX)) >> 3));
	uint16 di = si;
	si = (uint16)(si + 0x0A00);
	uint16 rows = (uint16)(((byte)(ctx.viewHalfH * 2)) << 5);
	uint16 cnt = (uint16)((((byte)(ctx.viewHalfW * 2)) + 1) << 2);
	for (uint16 r = rows; r != 0; r--) {
		copyBytes(di, si, cnt, false);
		di = (uint16)(di + kRowBytes);
		si = (uint16)(si + kRowBytes);
	}
}

void Video::scrollDown() {
	uint16 si = (uint16)(kRowBytes * (uint16)(0x8E + ctx.viewOffY + 0x1F));
	si = (uint16)(si + (((uint16)(0x130 - ctx.viewOffX)) >> 3));
	uint16 di = si;
	si = (uint16)(si - 0x0A00);
	uint16 rows = (uint16)(((byte)(ctx.viewHalfH * 2)) << 5);
	uint16 cnt = (uint16)((((byte)(ctx.viewHalfW * 2)) + 1) << 2);
	for (uint16 r = rows; r != 0; r--) {
		copyBytes(di, si, cnt, false);
		di = (uint16)(di - kRowBytes);
		si = (uint16)(si - kRowBytes);
	}
}

// ----------------------------------------------------------------------------
// 12 page flip, 14/19/1E/23 page copies, 15-17/1F-21 wipes
// ----------------------------------------------------------------------------

void Video::setPageAndDelay(byte page) {
	_displayed = page & 1;
	if (_host)
		_host->pageShown(_displayed);
	delay(ctx.delayStep);
}

void Video::showPage(byte page) {
	if (ctx.mousePresent != 0 && ctx.cursorHide >= 0) {
		if (ctx.mouseFlags & 0x80) {
			cursorRestoreBackground();
			ctx.mouseFlags &= 0x7F;
		}
		ctx.cursorPage = page;
		cursorSaveBackground();
		cursorStageShape();
		cursorBlitKeyed();
		setPageAndDelay(ctx.cursorPage);
	} else {
		ctx.cursorPage = page;
		setPageAndDelay(page);
	}
}

void Video::copyRectKeyed() {
	const uint16 sx1 = ctx.x1, sx3 = ctx.x3;
	uint16 outer = (uint16)(ctx.y2 - ctx.y1);
	const uint16 width = (uint16)(ctx.x2 - ctx.x1);
	do {
		uint16 inner = width;
		do {
			const byte c = getPixelAt(calcAddr(ctx.x1, ctx.y1));
			if (c != ctx.keyColour)
				putPixelAt(calcAddr(ctx.x3, ctx.y3), c);
			ctx.x3++;
			ctx.x1++;
		} while (--inner != 0);
		ctx.x1 = sx1;
		ctx.x3 = sx3;
		ctx.y1++;
		ctx.y3++;
	} while (--outer != 0);
}

void Video::copyScreen0To1AndShow() {
	copyBytes(0x8000, 0, (uint16)(kScreenHeight * kRowBytes), false);
	setPageAndDelay(1);
	if (ctx.mousePresent != 0 && ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.cursorPage = 1;
}

void Video::copyScreen1To0AndShow() {
	copyBytes(0, 0x8000, (uint16)(kScreenHeight * kRowBytes), false);
	setPageAndDelay(0);
	if (ctx.mousePresent != 0 && ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.cursorPage = 0;
}

void Video::copyRows0To1AndShow() {
	uint16 rows = (uint16)(ctx.y2 - ctx.y1);
	uint16 si = (uint16)((ctx.x1 >> 3) + ctx.y1 * kRowBytes);
	uint16 di = (uint16)(0x8000 + (ctx.x3 >> 3) + ctx.y3 * kRowBytes);
	for (uint16 r = rows; r != 0; r--) {
		copyBytes(di, si, kRowBytes, false);
		di = (uint16)(di + kRowBytes);
		si = (uint16)(si + kRowBytes);
	}
	if (ctx.mousePresent != 0 && ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.cursorPage = 1;
	setPageAndDelay(1);
}

void Video::copyRows1To0AndShow() {
	uint16 rows = (uint16)(ctx.y2 - ctx.y1);
	uint16 si = (uint16)(0x8000 + (ctx.x1 >> 3) + ctx.y1 * kRowBytes);
	uint16 di = (uint16)((ctx.x3 >> 3) + ctx.y3 * kRowBytes);
	for (uint16 r = rows; r != 0; r--) {
		copyBytes(di, si, kRowBytes, false);
		di = (uint16)(di + kRowBytes);
		si = (uint16)(si + kRowBytes);
	}
	if (ctx.mousePresent != 0 && ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.drawPage = 0;
	ctx.cursorPage = 0;
	setPageAndDelay(0);
}

namespace {

// copy one byte column of the 350 visible rows (movsb + add 4Fh per row)
inline void copyColumn(byte vram[4][kPlaneBytes], uint16 dst, uint16 src) {
	for (int r = 0; r < kScreenHeight; r++) {
		for (int p = 0; p < 4; p++)
			vram[p][(uint16)(dst + r * kRowBytes)] = vram[p][(uint16)(src + r * kRowBytes)];
	}
}

} // namespace

void Video::revealCenterOut0To1() {
	uint16 d1 = 0x8028, d2 = 0x8027, s1 = 0x28, s2 = 0x27;
	do {
		delay(ctx.delayStep);
		copyColumn(_vram, d1, s1);
		copyColumn(_vram, d2, s2);
		d1++; s1++; d2--; s2--;
	} while (d1 != 0x8050);
}

void Video::revealCenterOut1To0() {
	uint16 d1 = 0x28, d2 = 0x27, s1 = 0x8028, s2 = 0x8027;
	do {
		delay(ctx.delayStep);
		copyColumn(_vram, d1, s1);
		copyColumn(_vram, d2, s2);
		d1++; s1++; d2--; s2--;
	} while (d1 != 0x50);
}

void Video::revealEdgesIn0To1() {
	uint16 d1 = 0x8000, d2 = 0x804F, s1 = 0, s2 = 0x4F;
	do {
		delay(ctx.delayStep);
		copyColumn(_vram, d1, s1);
		copyColumn(_vram, d2, s2);
		d1++; s1++; d2--; s2--;
	} while (d1 != 0x8050);
}

void Video::revealEdgesIn1To0() {
	uint16 d1 = 0, d2 = 0x4F, s1 = 0x8000, s2 = 0x804F;
	do {
		delay(ctx.delayStep);
		copyColumn(_vram, d1, s1);
		copyColumn(_vram, d2, s2);
		d1++; s1++; d2--; s2--;
	} while (d1 != 0x50);
}

void Video::revealRightToLeft0To1() {
	uint16 d = 0x804F, s = 0x4F;
	do {
		delay(ctx.delayWipe);
		copyColumn(_vram, d, s);
		d--; s--;
	} while (s16(s) > 0);
}

void Video::revealRightToLeft1To0() {
	uint16 d = 0x4F, s = 0x804F;
	do {
		delay(ctx.delayWipe);
		copyColumn(_vram, d, s);
		d--; s--;
	} while (s16(s) > s16(0x8000));
}

// ----------------------------------------------------------------------------
// mouse cursor (sub_110A8, sub_11145, sub_10FEF, sub_11208, sub_114FD) and fn28-2D
// ----------------------------------------------------------------------------

namespace {

struct DrawStateSave {
	Context &c;
	byte page, draw;
	uint16 x1, y1, x2, y2, x3, y3;
	DrawStateSave(Context &ctx) : c(ctx), page(ctx.cursorPage), draw(ctx.drawPage),
		x1(ctx.x1), y1(ctx.y1), x2(ctx.x2), y2(ctx.y2), x3(ctx.x3), y3(ctx.y3) {
		c.drawPage = page;
	}
	void restore() {
		c.x1 = x1; c.y1 = y1; c.x2 = x2; c.y2 = y2; c.x3 = x3; c.y3 = y3;
		c.drawPage = draw;
	}
};

} // namespace

// capture the 16x16 under the pointer into slot 0 (saved background)
void Video::cursorSaveBackground() {
	DrawStateSave sv(ctx);
	ctx.x1 = (uint16)ctx.mouseX;
	ctx.curX = ctx.x1;
	ctx.x2 = (uint16)(ctx.x1 + 0x0F);
	ctx.y1 = (uint16)ctx.mouseY;
	ctx.curY = ctx.y1;
	ctx.y2 = (uint16)(ctx.y1 + 0x0F);
	ctx.image.set(cursorSlot(0), kCursorSlotBytes, 0);
	getImageImpl(true);
	ctx.imgRows = 0x20;
	ctx.imgBytes = 4;
	ctx.imgMask = 0xFF;
	sv.restore();
}

// draw the current cursor shape image to the off-screen staging area at (0,360)
void Video::cursorStageShape() {
	DrawStateSave sv(ctx);
	ctx.image.set(cursorSlot(ctx.cursorShape % kCursorSlots), kCursorSlotBytes, 0);
	ctx.x1 = 0;
	ctx.y1 = 0x168;
	ctx.x2 = 0x0F;
	ctx.y2 = 0x177;
	putImageHdr();
	sv.restore();
}

// copy the staged 16x16 to the cursor position, skipping colour 0
void Video::cursorBlitKeyed() {
	DrawStateSave sv(ctx);
	ctx.y1 = 0x168;
	ctx.y3 = ctx.curY;
	for (int r = 0; r < 16; r++) {
		ctx.x1 = 0;
		ctx.x3 = ctx.curX;
		for (int c = 0; c < 16; c++) {
			const byte col = getPixelAt(calcAddr(ctx.x1, ctx.y1));
			if (col != 0)
				putPixelAt(calcAddr(ctx.x3, ctx.y3), col);
			ctx.x3++;
			ctx.x1++;
		}
		ctx.y1++;
		ctx.y3++;
	}
	sv.restore();
}

// put the saved background back at the position where the cursor is drawn
void Video::cursorRestoreBackground() {
	DrawStateSave sv(ctx);
	ctx.image.set(cursorSlot(0), kCursorSlotBytes, 0);
	ctx.x1 = ctx.curX;
	ctx.x2 = (uint16)(ctx.x1 + 0x0F);
	ctx.y1 = ctx.curY;
	ctx.y2 = (uint16)(ctx.y1 + 0x0F);
	putImageHdr();
	sv.restore();
}

void Video::mouseShow() {
	if (ctx.mousePresent == 0)
		return;
	ctx.cursorHide++;
	if (ctx.cursorHide < 0)
		return;
	ctx.cursorHide = 0;
	cursorSaveBackground();
	cursorStageShape();
	cursorBlitKeyed();
}

void Video::mouseHide() {
	if (ctx.mousePresent == 0)
		return;
	if (ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.cursorHide--;
}

void Video::mouseSetBounds() {
	if (ctx.mousePresent == 0)
		return;
	if (ctx.cursorHide >= 0)
		cursorRestoreBackground();
	ctx.clipX1 = s16(ctx.x1);
	if (ctx.mouseX < ctx.clipX1) ctx.mouseX = ctx.clipX1;
	ctx.clipX2 = s16(ctx.x2);
	if (ctx.mouseX > ctx.clipX2) ctx.mouseX = ctx.clipX2;
	ctx.clipY1 = s16(ctx.y1);
	if (ctx.mouseY < ctx.clipY1) ctx.mouseY = ctx.clipY1;
	ctx.clipY2 = s16(ctx.y2);
	if (ctx.mouseY > ctx.clipY2) ctx.mouseY = ctx.clipY2;
	if (ctx.cursorHide >= 0) {
		cursorSaveBackground();
		cursorStageShape();
		cursorBlitKeyed();
	}
}

void Video::mouseRefresh() {
	if (ctx.mousePresent == 0)
		return;
	if (ctx.cursorHide >= 0) {
		if (!(ctx.mouseFlags & 0x80))
			return;
		cursorRestoreBackground();
		cursorSaveBackground();
		cursorStageShape();
		cursorBlitKeyed();
	}
	ctx.mouseFlags &= 0x7F;
}

void Video::mouseSetPos() {
	if (ctx.mousePresent == 0)
		return;
	if (ctx.cursorHide >= 0)
		cursorRestoreBackground();
	int16 x = s16(ctx.x1);
	if (x < ctx.clipX1) x = ctx.clipX1;
	if (x > ctx.clipX2) x = ctx.clipX2;
	ctx.mouseX = x;
	int16 y = s16(ctx.y1);
	if (y < ctx.clipY1) y = ctx.clipY1;
	if (y > ctx.clipY2) y = ctx.clipY2;
	ctx.mouseY = y;
	if (ctx.cursorHide >= 0) {
		cursorSaveBackground();
		cursorStageShape();
		cursorBlitKeyed();
	}
	ctx.mouseFlags &= 0x7F;
}

void Video::mouseSetCursorMode(byte mode) {
	if (!(ctx.mousePresent & 0x80))   // the original tests the status byte as signed: FFh = present
		return;
	ctx.cursorMode = mode;
	cursorUpdateShape();
	if (ctx.cursorHide >= 0) {
		cursorRestoreBackground();
		cursorSaveBackground();
		cursorStageShape();
		cursorBlitKeyed();
	}
}

// the INT 33h callback (mask 0Bh): button presses set flag bits, movement updates the position
void Video::mouseEvent(bool moved, int dx, int dy, int leftPresses, int rightPresses) {
	if (ctx.mousePresent == 0)
		return;
	if (leftPresses != 0)
		ctx.mouseFlags |= 0x60;
	if (rightPresses != 0)
		ctx.mouseFlags |= 0x42;
	if (!moved) {
		ctx.mouseFlags &= 0x7F;
		return;
	}
	ctx.mouseFlags |= 0x80;
	int x = ctx.mouseX + dx;
	if (x < ctx.clipX1) x = ctx.clipX1;
	if (x > ctx.clipX2) x = ctx.clipX2;
	ctx.mouseX = (int16)x;
	int y = ctx.mouseY + dy;
	if (y < ctx.clipY1) y = ctx.clipY1;
	if (y > ctx.clipY2) y = ctx.clipY2;
	ctx.mouseY = (int16)y;
	cursorUpdateShape();
}

// sub_114FD: pick the cursor shape (1..12) from the cursor mode and pointer position.
void Video::cursorUpdateShape() {
	Context &c = ctx;
	const int mx = c.mouseX, my = c.mouseY;
	if (c.cursorMode == 0) {
		c.cursorShape = 1;
		return;
	}
	if (c.cursorMode == 1) {
		c.cursorShape = 1;
		if ((int16)(c.hotX - c.hotMarginX) >= mx) return;
		if (mx < 0x30) return;
		if ((int16)(c.hotX + 0x10 + c.hotMarginX) < mx) return;
		if (mx > 0x23F) return;
		if ((int16)(c.hotY - c.hotMarginY) >= my) return;
		if (my < 0x0E) return;
		if ((int16)(c.hotY + 0x10 + c.hotMarginY) < my) return;
		if (my > 0x11D) return;
		c.cursorShape = 2;
		return;
	}
	if (c.cursorMode != 2) {
		c.cursorMode = 0;
		c.cursorShape = 1;
		return;
	}
	if (c.flags912 & 0xC0) {
		c.cursorShape = 1;
		if (mx < 0x20 || my < -2 || mx > 0x24F || my > 0x12D)
			return;
		int ax = c.hotX - 0x20;
		if (mx < ax) return;
		ax += 0x60;
		if (mx > ax) return;
		int ay = c.hotY - 0x20;
		if (my < ay) return;
		ay += 0x60;
		if (my > ay) return;
		uint16 axu = (uint16)(c.hotX + 9);
		uint16 bxu = (uint16)(c.hotY + 9);
		const uint16 mxu = (uint16)mx, myu = (uint16)my;
		if (mxu > axu) {
			axu = (uint16)(axu + 0x0D);
			if (mxu <= axu) {
				bxu = (uint16)(bxu - 6);
				c.cursorShape = myu > bxu ? 6 : 4;
				return;
			}
			if (myu <= bxu) {
				c.cursorShape = 9;
				return;
			}
			bxu = (uint16)(bxu + 0x0D);
			c.cursorShape = myu >= bxu ? 5 : 0x0A;
			return;
		}
		if (myu <= bxu) {
			c.cursorShape = 3;
			return;
		}
		bxu = (uint16)(bxu + 0x0D);
		c.cursorShape = myu >= bxu ? 0x0B : 0x0C;
		return;
	}
	// mode 2, regions of the play area
	c.cursorShape = 1;
	if (mx < 0x30 || my < 0x0E || mx > 0x23F || my > 0x11D)
		return;
	const int bh = (mx - 0x30) >> 5;
	const int bl = (my - 0x0E) >> 5;
	const int bx = (bh << 8) | bl;
	if (bl == 4) {
		c.cursorShape = bh > 8 ? 0x0A : 0x0C;
		return;
	}
	if (bl < 4) {
		if (bh == 8) { c.cursorShape = 4; return; }
		if (bh < 8) {
			if (bl >= 3) { c.cursorShape = bh > 5 ? 3 : 0x0C; return; }
			if (bh <= 5 || bx == 0x602) { c.cursorShape = 3; return; }
			c.cursorShape = 4;
			return;
		}
		// bh > 8
		if (bl >= 3) { c.cursorShape = bh < 0x0B ? 9 : 0x0A; return; }
		if (bh >= 0x0B || bx == 0x0A02) { c.cursorShape = 9; return; }
		c.cursorShape = 4;
		return;
	}
	// bl > 4
	if (bh == 8) { c.cursorShape = 6; return; }
	if (bh < 8) {
		if (bl <= 5) { c.cursorShape = bh > 5 ? 0x0B : 0x0C; return; }
		if (bh <= 5 || bx == 0x606) { c.cursorShape = 0x0B; return; }
		c.cursorShape = 6;
		return;
	}
	// bh > 8
	if (bl <= 5) { c.cursorShape = bh < 0x0B ? 5 : 0x0A; return; }
	if (bh >= 0x0B || bx == 0x0A06) { c.cursorShape = 5; return; }
	c.cursorShape = 6;
}

// ----------------------------------------------------------------------------
// INT 62h dispatch
// ----------------------------------------------------------------------------

void Video::call(byte ah, byte al) {
	switch (ah) {
	case 0x00: init(); break;
	case 0x01: shutdown(); break;
	case 0x02: setGraphicsMode(); break;
	case 0x03: putPixel(); break;
	case 0x04: getPixel(); break;
	case 0x05: line(); break;
	case 0x06: rect(); break;
	case 0x07: fillRect(); break;
	case 0x08: drawChar(); break;
	case 0x09: getImageRaw(); break;
	case 0x0A: putImage(); break;
	case 0x0B: getImage(); break;
	case 0x0C: putImageHdr(); break;
	case 0x0D: copyTileRow(al); break;
	case 0x0E: scrollRight(); break;
	case 0x0F: scrollLeft(); break;
	case 0x10: scrollUp(); break;
	case 0x11: scrollDown(); break;
	case 0x12: showPage(al); break;
	case 0x13: copyRectKeyed(); break;
	case 0x14: copyScreen0To1AndShow(); break;
	case 0x15: revealCenterOut0To1(); break;
	case 0x16: revealRightToLeft0To1(); break;
	case 0x17: revealEdgesIn0To1(); break;
	case 0x19: copyRows0To1AndShow(); break;
	case 0x1E: copyScreen1To0AndShow(); break;
	case 0x1F: revealCenterOut1To0(); break;
	case 0x20: revealRightToLeft1To0(); break;
	case 0x21: revealEdgesIn1To0(); break;
	case 0x23: copyRows1To0AndShow(); break;
	case 0x28: mouseShow(); break;
	case 0x29: mouseHide(); break;
	case 0x2A: mouseSetBounds(); break;
	case 0x2B: mouseRefresh(); break;
	case 0x2C: mouseSetPos(); break;
	case 0x2D: mouseSetCursorMode(al); break;
	default: break;   // numbers the driver has no handler for do nothing
	}
}

} // namespace Vid
} // namespace Yendor1

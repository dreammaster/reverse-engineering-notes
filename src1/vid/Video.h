/* Yendor1::Vid -- the video driver (VID16.DRV, INT 62h) reimplemented.
 *
 * The original is a resident driver whose callers write their parameters into a
 * "context block" (docs1/video-api.md) and then execute INT 62h with the function
 * number in AH. Here the context block is the public `ctx` member, the function
 * number selects a method, and the EGA planar video memory is emulated exactly
 * (4 planes, two 32 KB pages, 80 bytes per scanline) so the routines can be
 * transliterated at byte level, including their quirks.
 *
 * Nothing in here touches a platform API: presenting the pages is the host's job
 * (see render()). */

#ifndef YENDOR1_VID_VIDEO_H
#define YENDOR1_VID_VIDEO_H

#include "shared/Types.h"

namespace Yendor1 {
namespace Vid {

using Shared::byte;
using Shared::int16;
using Shared::uint16;

enum {
	kScreenWidth = 640,
	kScreenHeight = 350,
	kRowBytes = 80,
	kPageBytes = 0x8000,     ///< per plane; page 1 starts at 0x8000
	kPlaneBytes = 0x10000,
	kCursorSlots = 13,
	kCursorSlotBytes = 0x85  ///< 5-byte header + 4 planes x 16 rows x 2 bytes
};

/** A far pointer into a caller-owned buffer (the driver's ctx+16h:18h). */
struct ImagePtr {
	byte *data;
	uint32_t size;
	uint32_t offset;
	ImagePtr() : data(0), size(0), offset(0) {}
	void set(byte *d, uint32_t sz, uint32_t off = 0) { data = d; size = sz; offset = off; }
};

/** Callbacks into the host: busy-wait loops and page flips. */
class Host {
public:
	virtual ~Host() {}
	/** A busy-wait of `loopCount` iterations of the original's `loop` instruction. */
	virtual void delay(uint16 loopCount) { (void)loopCount; }
	/** The displayed page changed (INT 10h AH=5). */
	virtual void pageShown(int page) { (void)page; }
};

/** Named view of the driver's context block. Offsets in comments are the original's. */
struct Context {
	uint16 tileSlots;      ///< 00h  tile cache slots (the clients' tile cache, see docs1)
	uint16 delayStep;      ///< 08h  busy-wait count after page flips / per wipe step
	uint16 delayWipe;      ///< 0Ah  busy-wait count per right-to-left wipe column
	uint16 delayCpu;       ///< 0Ch  third delay count set by SW from the CPU class
	uint16 cpuClass;       ///< 0Eh  detectCpuClass result stored by SW
	uint16 imgRows;        ///< 10h  image height in rows (default 32)
	uint16 imgBytes;       ///< 12h  image width in bytes per plane row (default 4)
	byte imgMask;          ///< 14h  right-edge pixel mask (default FFh)
	ImagePtr image;        ///< 16h:18h far pointer to image data
	uint16 x1, y1, x2, y2; ///< 1Ah 1Ch 1Eh 20h
	uint16 x3, y3;         ///< 22h 24h
	byte textCol, textRow; ///< 2Ah 2Bh
	byte textChar;         ///< 2Ch
	byte textFg, textBg;   ///< 2Eh 2Fh
	byte colour;           ///< 30h
	byte drawPage;         ///< 31h
	byte destPage;         ///< 32h
	byte videoKind;        ///< 33h  1 = EGA 640x350 (the only mode implemented), 2 = SVGA
	byte keyColour;        ///< 34h
	byte fontSelect;       ///< 35h  0 = 8x14 font, else 8x8
	byte paletteCommand;   ///< 676h command mailbox read by the palette helper
	byte viewHalfW;        ///< 678h map view size for the scroll calls
	byte viewHalfH;        ///< 679h
	uint16 hotMarginX;     ///< 67Ah cursor hot-box margins
	uint16 hotMarginY;     ///< 67Ch
	uint16 viewOffX;       ///< 67Eh scroll geometry
	uint16 viewOffY;       ///< 680h
	byte flags912;         ///< 912h game-set flags (cursor mode 2 variant)
	byte mousePresent;     ///< 952h
	byte mouseFlags;       ///< 953h bit7 moved; left press sets 60h; right press sets 42h
	int8_t cursorHide;     ///< 955h show counter, -1 = hidden
	byte cursorShape;      ///< 956h
	byte cursorPage;       ///< 957h page the cursor is drawn on
	byte cursorMode;       ///< 958h
	int16 clipX1, clipX2;  ///< 95Ah 95Ch
	int16 clipY1, clipY2;  ///< 95Eh 960h
	uint16 curX, curY;     ///< 962h 964h where the cursor is currently drawn
	int16 mouseX, mouseY;  ///< 966h 968h
	uint16 hotX, hotY;     ///< 96Ah 96Ch
	Context();
};

class Video {
public:
	Video();

	Context ctx;

	void setHost(Host *host) { _host = host; }
	/** ROM font (256 glyphs of `height` bytes each) used by drawChar. Not copied. */
	void setFont8x14(const byte *data) { _font14 = data; }
	void setFont8x8(const byte *data) { _font8 = data; }
	/** EGA palette registers (6-bit rgbRGB) used by render(). */
	void setPaletteRegisters(const byte regs[16]);

	/** The cursor image store: 13 slots of kCursorSlotBytes. Slot 0 holds the saved background;
	 *  slots 1-12 are the cursor shapes, filled in by the game with the 5-byte header + planar data. */
	byte *cursorStore() { return _cursorStore; }

	// ---- AH = function number (docs1/video-api.md) ----
	void init(bool mousePresent = true);  // 00 (the driver state; the segment return value has no equivalent)
	void shutdown();                      // 01
	void setGraphicsMode();               // 02 clears the video memory
	void putPixel();                      // 03
	void getPixel();                      // 04
	void line();                          // 05
	void rect();                          // 06
	void fillRect();                      // 07
	void drawChar();                      // 08
	void getImageRaw();                   // 09
	void putImage();                      // 0A
	void getImage();                      // 0B
	void putImageHdr();                   // 0C
	void copyTileRow(byte tiles);         // 0D
	void scrollRight();                   // 0E
	void scrollLeft();                    // 0F
	void scrollUp();                      // 10
	void scrollDown();                    // 11
	void showPage(byte page);             // 12
	void copyRectKeyed();                 // 13
	void copyScreen0To1AndShow();         // 14
	void revealCenterOut0To1();           // 15
	void revealRightToLeft0To1();         // 16
	void revealEdgesIn0To1();             // 17
	void copyRows0To1AndShow();           // 19
	void copyScreen1To0AndShow();         // 1E
	void revealCenterOut1To0();           // 1F
	void revealRightToLeft1To0();         // 20
	void revealEdgesIn1To0();             // 21
	void copyRows1To0AndShow();           // 23
	void mouseShow();                     // 28
	void mouseHide();                     // 29
	void mouseSetBounds();                // 2A
	void mouseRefresh();                  // 2B
	void mouseSetPos();                   // 2C
	void mouseSetCursorMode(byte mode);   // 2D

	/** Dispatch by AH exactly like the INT 62h service routine; AL is the argument for 0D, 12 and 2D.
	 *  Unimplemented numbers are ignored (as in the original). */
	void call(byte ah, byte al = 0);

	/** The host's emulation of the INT 33h event handler (vid_mouseEventHandler):
	 *  `moved` with the motion counters since the last call, and the button presses since the last call. */
	void mouseEvent(bool moved, int dx, int dy, int leftPresses, int rightPresses);

	// ---- inspection / presentation ----
	int displayedPage() const { return _displayed; }
	/** Colour index (0-15) of a pixel on a page, for tests and rendering. */
	byte pixel(int page, int x, int y) const;
	/** Convert a page to 8-bit colour indices (640x350, row-major). */
	void renderIndexed(int page, byte *out) const;
	/** Convert a page to RGB through the EGA palette registers. */
	void renderRgb(int page, byte *out) const;
	/** Raw plane byte (page-relative offset), for tests. */
	byte plane(int p, uint16 offset) const { return _vram[p][offset]; }
	void setPlane(int p, uint16 offset, byte v) { _vram[p][offset] = v; }

private:
	// video memory: 4 planes of 64 KB (page 0 = 0000h-7FFFh, page 1 = 8000h-FFFFh)
	byte _vram[4][kPlaneBytes];
	byte _cursorStore[kCursorSlots * kCursorSlotBytes + 16];
	byte _palette[16];
	const byte *_font14;
	const byte *_font8;
	Host *_host;
	int _displayed;
	int _scratchRows;

	// internals (ports of the driver's helper routines)
	struct Addr {
		uint16 offset;
		int shift;     // 7 - (x & 7), the original's CL after calcVramAddr
	};
	Addr calcAddr(int x, int y) const;
	void putPixelAt(const Addr &a, byte colour);
	byte getPixelAt(const Addr &a) const;
	void storeMasked(int plane, uint16 offset, byte mask, byte value);
	void lineImpl();
	void putImageImpl();
	void getImageImpl(bool withHeader);
	void drawCharImpl();
	void copyBytes(uint16 dst, uint16 src, uint16 count, bool backward);
	void setPageAndDelay(byte page);
	void delay(uint16 n);
	// mouse cursor
	void cursorBlitKeyed();
	void cursorSaveBackground();
	void cursorStageShape();
	void cursorRestoreBackground();
	void cursorUpdateShape();
	byte *cursorSlot(int index) { return _cursorStore + index * kCursorSlotBytes; }

	Video(const Video &);
	Video &operator=(const Video &);
};

} // namespace Vid
} // namespace Yendor1

#endif

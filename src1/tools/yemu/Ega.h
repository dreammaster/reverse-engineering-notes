/* EGA/VGA planar graphics hardware as seen by the CPU emulator: the four 64 KB planes at A000:0000, the sequencer
 * map mask, and the graphics controller (set/reset, colour compare, rotate/function, read map select, mode, bit mask)
 * with write modes 0-2 and read modes 0-1. */

#ifndef YEMU_EGA_H
#define YEMU_EGA_H

#include <stdint.h>

namespace Yemu {

class Ega {
public:
	Ega();

	uint8_t plane[4][65536];
	uint8_t egaPalette[17];     // last INT 10h/1002h table (16 registers + overscan)
	int displayedPage;          // INT 10h AH=5
	int videoMode;

	uint8_t read(uint32_t offset);
	void write(uint32_t offset, uint8_t v);
	uint8_t inPort(uint16_t port);
	void outPort(uint16_t port, uint8_t v);

	/** Colour (0-15) of the pixel (x, y) on a page, 640 pixels per row of 80 bytes. */
	uint8_t pixel(int page, int x, int y) const;

private:
	uint8_t seqIndex, mapMask;
	uint8_t gcIndex;
	uint8_t setReset, enableSetReset, colourCompare, rotateFunc, readMap, mode, miscGc, colourDontCare, bitMask;
	uint8_t latch[4];
	unsigned statusToggle;
};

} // namespace Yemu

#endif

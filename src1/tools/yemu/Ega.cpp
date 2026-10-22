#include "Ega.h"

#include <string.h>

namespace Yemu {

Ega::Ega()
	: displayedPage(0), videoMode(3), seqIndex(0), mapMask(0x0F), gcIndex(0),
	  setReset(0), enableSetReset(0), colourCompare(0), rotateFunc(0), readMap(0), mode(0), miscGc(0), colourDontCare(0x0F), bitMask(0xFF),
	  statusToggle(0) {
	memset(plane, 0, sizeof(plane));
	memset(latch, 0, sizeof(latch));
	memset(egaPalette, 0, sizeof(egaPalette));
}

uint8_t Ega::read(uint32_t off) {
	off &= 0xFFFF;
	for (int p = 0; p < 4; p++)
		latch[p] = plane[p][off];
	if (!(mode & 0x08))
		return latch[readMap & 3];
	// read mode 1: colour compare
	uint8_t result = 0xFF;
	for (int p = 0; p < 4; p++) {
		if (!(colourDontCare & (1 << p)))
			continue;
		const uint8_t want = (colourCompare & (1 << p)) ? 0xFF : 0x00;
		result &= (uint8_t)~(latch[p] ^ want);
	}
	return result;
}

void Ega::write(uint32_t off, uint8_t data) {
	off &= 0xFFFF;
	const int wm = mode & 3;
	if (wm == 1) {
		for (int p = 0; p < 4; p++)
			if (mapMask & (1 << p))
				plane[p][off] = latch[p];
		return;
	}
	for (int p = 0; p < 4; p++) {
		if (!(mapMask & (1 << p)))
			continue;
		uint8_t v;
		if (wm == 0) {
			const int rot = rotateFunc & 7;
			const uint8_t d = rot ? (uint8_t)((data >> rot) | (data << (8 - rot))) : data;
			v = (enableSetReset & (1 << p)) ? ((setReset & (1 << p)) ? 0xFF : 0x00) : d;
		} else {   // write mode 2: data bit p is the value for plane p
			v = (data & (1 << p)) ? 0xFF : 0x00;
		}
		switch ((rotateFunc >> 3) & 3) {
		case 1: v &= latch[p]; break;
		case 2: v |= latch[p]; break;
		case 3: v ^= latch[p]; break;
		default: break;
		}
		v = (uint8_t)((v & bitMask) | (latch[p] & ~bitMask));
		plane[p][off] = v;
	}
}

uint8_t Ega::inPort(uint16_t port) {
	switch (port) {
	case 0x3DA:                 // input status 1: vertical retrace / display enable toggle so wait loops end
		statusToggle++;
		return (uint8_t)(((statusToggle >> 1) & 1) ? 0x09 : 0x00);
	case 0x3C5: return seqIndex == 2 ? mapMask : 0;
	case 0x3CF:
		switch (gcIndex) {
		case 0: return setReset;
		case 1: return enableSetReset;
		case 2: return colourCompare;
		case 3: return rotateFunc;
		case 4: return readMap;
		case 5: return mode;
		case 6: return miscGc;
		case 7: return colourDontCare;
		case 8: return bitMask;
		default: return 0;
		}
	default: return 0;
	}
}

void Ega::outPort(uint16_t port, uint8_t v) {
	switch (port) {
	case 0x3C4: seqIndex = v; break;
	case 0x3C5: if (seqIndex == 2) mapMask = v & 0x0F; break;
	case 0x3CE: gcIndex = v & 0x0F; break;
	case 0x3CF:
		switch (gcIndex) {
		case 0: setReset = v & 0x0F; break;
		case 1: enableSetReset = v & 0x0F; break;
		case 2: colourCompare = v & 0x0F; break;
		case 3: rotateFunc = v; break;
		case 4: readMap = v & 3; break;
		case 5: mode = v; break;
		case 6: miscGc = v; break;
		case 7: colourDontCare = v & 0x0F; break;
		case 8: bitMask = v; break;
		default: break;
		}
		break;
	default: break;
	}
}

uint8_t Ega::pixel(int page, int x, int y) const {
	const uint16_t off = (uint16_t)((page ? 0x8000 : 0) + y * 80 + (x >> 3));
	const uint8_t m = (uint8_t)(0x80 >> (x & 7));
	uint8_t c = 0;
	for (int p = 0; p < 4; p++)
		if (plane[p][off] & m)
			c |= (uint8_t)(1 << p);
	return c;
}

} // namespace Yemu

#include "wasteland1/movement.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/interact.h"
#include "wasteland1/legacy.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/party.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
uint8_t &S(Machine &m, uint8_t o) { return m.d8(uint16_t(g::saved2 + o)); }
uint8_t rec(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationDetailsPtr) + i)); }
uint8_t mapByte(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationsPtr) + i)); }
}

void advanceTime(Machine &m) {
	const unsigned a = S(m, 0x0A) + mapByte(m, 0x34);
	S(m, 0x0A) = uint8_t(a);
	const unsigned b = S(m, 0x0B) + mapByte(m, 0x35) + (a >> 8);
	S(m, 0x0B) = uint8_t(b);
	if (uint8_t(b) >= 0x3C) {
		S(m, 0x0B) = uint8_t(uint8_t(b) - 0x3C);
		const uint8_t hour = uint8_t(S(m, 0x0C) + 1);
		S(m, 0x0C) = hour >= 0x18 ? 0 : hour;
	}
	m.d8(0x4722) = uint8_t(m.d8(0x4722) + mapByte(m, 0x36));
	const unsigned c = S(m, 2) + mapByte(m, 0x36);
	S(m, 2) = uint8_t(c);
	if (c > 0xFF && ++S(m, 3) == 0 && ++S(m, 4) == 0) ++m.d8(uint16_t(g::saved + 0xFD));
	if ((m.d8(0x4722) & 0x0F) == 0) healthTick(m);
}

bool cellBlockedByObject(Machine &m, uint8_t x, uint8_t y) {
	m.d8(0xA5D8) = x;
	m.d8(0xA5D7) = y;
	for (;;) {
		x = m.d8(0xA5D8);
		y = m.d8(0xA5D7);
		if (selectCellLocation(m, x, y) != 2) return false;
		if (rec(m, 0) & 0x80) return false;
		uint8_t result = 0;
		if (evaluateAction(m, m.d8(0xA5D8), m.d8(0xA5D7), result)) continue;
		return result != 0;
	}
}

bool confirmGate(Machine &m, uint8_t x, uint8_t y) {
	if (selectCellLocation(m, x, y) == 0x0A && (rec(m, 0) & 0x40)) {
		X86Regs r;
		r.al = rec(m, 0) & 0x3F;
		legacy::call(m, 0x7920, r);   // sub_17920 (through sub_16B17)
		selectWriterForRoster(m);
		printMessage(m, 0x67);
		X86Regs yn;
		legacy::call(m, 0x2619, yn);   // sub_12619: yes / no
		if (yn.cf) {
			X86Regs layout;
			legacy::call(m, 0x63C4, layout);   // sub_163C4
			m.d8(0xAAD3) = 0;
			m.d8(0xAAD4) = 1;
			return true;
		}
		m.d8(0xAAD3) = 1;
	}
	m.d8(0xAAD4) = 0;
	return false;
}

bool applyStepOnChange(Machine &m, uint8_t x, uint8_t y) {
	m.d8(0xA6B0) = x;
	m.d8(0xA6B1) = y;
	const uint8_t cls = selectCellLocation(m, x, y);
	uint8_t type;
	if (cls == 0x0B) type = 1;
	else if (cls == 4 && (rec(m, 1) & 0x80)) type = 2;
	else return false;
	m.d8(0xA6B2) = type;
	showRecordString(m, 0);
	if (!(rec(m, type) & 0x80)) {
		X86Regs r;
		r.al = type;
		r.dl = x;
		r.bl = y;
		legacy::call(m, 0x7CFF, r);   // sub_17CFF
	}
	return true;
}

void stepTarget(uint8_t dir, uint8_t &x, uint8_t &y) {
	switch (dir) {
	case 0: --y; break;
	case 1: ++y; break;
	case 2: --x; break;
	case 3: ++x; break;
	case 5: --y; --x; break;
	case 6: ++x; --y; break;
	case 7: --x; ++y; break;
	case 8: ++y; ++x; break;
	default: break;
	}
}

bool stepBlocked(Machine &m, uint8_t dir) {
	uint8_t x, y;
	partyCell(m, x, y);
	stepTarget(dir, x, y);
	if (outsideMap(m, x, y)) return true;
	m.d8(0xAA4B) = x;
	m.d8(0xAA4A) = y;
	const uint8_t cls = tileClassAt(m, x, y);
	if (cls == 0x0F || cls == 3) return true;
	if (cellBlockedByObject(m, x, y)) return true;
	if (confirmGate(m, x, y)) return true;
	return applyStepOnChange(m, x, y);
}

void refreshViewLayout(Machine &m) {
	if (m.d8(0x4724) != 0) return;
	if ((m.d8(0x4707) | m.d8(0x46B9)) == 0) return;
	X86Regs r;
	legacy::call(m, 0x63C4, r);   // sub_163C4
}

bool moveParty(Machine &m, uint8_t dir) {
	m.d8(0xAA47) = dir;
	refreshViewLayout(m);
	if (countConsciousMembers(m) == 0) return false;
	refreshViewLayout(m);
	if (m.d8(0x4724) == 0) drawPartyCell(m);
	switch (dir) {
	case 0: scrollViewUp(m); break;
	case 1: scrollViewDown(m); break;
	case 2: scrollViewLeft(m); break;
	case 3: scrollViewRight(m); break;
	default: break;
	}
	{
		uint8_t x, y;
		partyCell(m, x, y);
		m.d8(0x46A6) = x;
		m.d8(0x46A7) = y;
	}
	advanceTime(m);
	return true;
}

bool tryStep(Machine &m, uint8_t dir) {
	m.d8(0x916B) = uint8_t(dir - 4);
	refreshViewLayout(m);
	if (stepBlocked(m, dir)) return true;
	return !moveParty(m, dir);
}

} // namespace wl

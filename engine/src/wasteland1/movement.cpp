#include "wasteland1/movement.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/interact.h"
#include "wasteland1/legacy.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
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


void randomEncounter(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	auto map = [&](uint8_t i) -> uint8_t & { return D(uint16_t(m.d16(g::locationsPtr) + i)); };
	auto rec = [&](uint8_t i) -> uint8_t & { return D(uint16_t(m.d16(g::locationDetailsPtr) + i)); };
	constexpr uint16_t slotVar = 0x46B4, kindVar = 0xA63A, range = 0xA634, steps = 0xA635, firstDir = 0xA636, dirVar = 0xA637;
	Rules rules(m);
	if (map(0x2F) == 0) return;
	if (rules.random(map(0x2F)) != 1) return;
	uint8_t slot = 0;
	for (;;) {   // 168A5: a free encounter slot in the map's list
		D(slotVar) = slot;
		selectLocationRecord(m, 0x0F, slot);
		if ((rec(3) | rec(5) | rec(7)) == 0) break;
		slot = uint8_t(D(slotVar) + 1);
		if (slot == map(0x32)) return;
	}
	D(kindVar) = rules.random(map(0x31));   // 168CF: which monster
	getMonsterDetails(m, D(kindVar));
	const uint8_t table = itemDamageDice(m);
	rec(0) = D(uint16_t(0xAA60 + table));
	rec(1) = D(uint16_t(0xAA6D + table));
	D(range) = D(uint16_t(0xAA7A + table));
	D(firstDir) = uint8_t(rules.random(9) - 1);
	uint8_t dir = D(firstDir);
	for (;;) {
		D(dirVar) = dir;
		if (dir != 4) {   // 16915: walk `range` cells in that direction
			uint8_t x = D(0x46A6), y = D(0x46A7);
			D(steps) = D(range);
			bool ok = true;
			do {
				stepTarget(D(dirVar), x, y);
				if (outsideMap(m, x, y) || partyAtCell(m, x, y)) { ok = false; break; }
				D(0xA639) = x;
				D(0xA638) = y;
				if (x == 0 || y == 0 || tileClassAt(m, x, y) != 0) { ok = false; break; }
			} while (--D(steps) != 0);
			if (ok) {   // room for the monsters here
				rec(4) = rules.random(uint8_t(m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 4)) >> 4));
				rec(3) = D(kindVar);
				D(0x46B3) = 0x0F;
				applyCellChange(m, x, y);
				return;
			}
		}
		dir = uint8_t(D(dirVar) + 1);   // 16983: next direction around the compass
		for (;;) {
			if (dir == D(firstDir)) {
				if (--D(range) == 0) return;   // 1699B: try again a shorter way
				break;
			}
			if (dir < 9) break;
			dir = 0;
		}
	}
}


bool enterLocationMove(Machine &m, uint8_t al) {   // 16A13 (returns cf through the port): a party step onto a location cell
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	auto finishLoad = [&](uint8_t locationId) {   // 16AC7
		D(0x4655) = locationId;
		X86Regs x;
		legacy::call(m, 0x8350, x);
		legacy::call(m, 0x63CA, x);
		mergeGroupsAtSamePosition(m);   // 16AD0
	};
	if (al != 0) return false;
	if (D(0xAAD3) == 0) {
		X86Regs x;
		x.al = uint8_t(m.d8(m.d16(g::locationDetailsPtr)) & 0x3F);
		legacy::call(m, 0x7920, x);   // 16B17
	}
	D(0xAAD3) = 0;
	changeCellFromLocation(m, 4, D(0x46A6), D(0x46A7));   // 169B1
	const uint16_t p = m.d16(g::partyIndexesPtr);
	D(0xAAD1) = D(uint16_t(p + 11));
	D(uint16_t(p + 11)) = D(0x46A6);
	D(0xAAD0) = D(uint16_t(p + 12));
	D(uint16_t(p + 12)) = D(0x46A7);
	D(0xAAD2) = D(uint16_t(p + 13));
	D(uint16_t(p + 13)) = D(0x4655);
	const uint16_t loc = m.d16(g::locationDetailsPtr);
	uint8_t dx, dy;
	if (D(uint16_t(loc + 3)) == 0xFF) {   // 16A75: come back from where the party was
		X86Regs x;
		x.dl = D(0xAAD1);
		x.bl = D(0xAAD0);
		legacy::call(m, 0x7DE0, x);
		D(0x464E) = x.dl;
		D(0x464F) = x.bl;
		finishLoad(D(0xAAD2));
		return false;
	}
	if (int8_t(D(loc)) >= 0) {
		dx = 0;
		dy = 0;
	} else {
		dx = D(0x46A6);
		dy = D(0x46A7);
	}
	D(0xAAD1) = dx;
	D(0xAAD0) = dy;
	X86Regs x;
	x.dl = uint8_t(D(uint16_t(m.d16(g::locationDetailsPtr) + 1)) + D(0xAAD1));
	x.bl = uint8_t(D(uint16_t(m.d16(g::locationDetailsPtr) + 2)) + D(0xAAD0));
	legacy::call(m, 0x7DE0, x);
	D(0x464E) = x.dl;
	D(0x464F) = x.bl;
	finishLoad(D(uint16_t(m.d16(g::locationDetailsPtr) + 3)));
	return true;
}

} // namespace wl

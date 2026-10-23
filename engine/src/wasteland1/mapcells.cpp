#include "wasteland1/mapcells.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/combat.h"

namespace wl {

namespace {

// Row offset of the cell tables: `row` shifted left by the position of the lowest set bit of (width / 2^skip). The
// original derives it by shifting `width` right until a one bit falls out; `skip` is the number of bits it examines first.
bool rowShift(uint8_t width, int firstShifts, uint8_t row, uint16_t &result) {
	uint8_t w = width;
	for (int i = 0; i < firstShifts; ++i) {
		const bool carry = w & 1;
		w >>= 1;
		if (carry) return false;
	}
	uint16_t v = row;
	for (int guard = 0; guard < 16; ++guard) {
		v = uint16_t(v << 1);
		const bool carry = w & 1;
		w >>= 1;
		if (carry) break;
	}
	result = v;
	return true;
}

}

uint8_t tileClassAt(Machine &m, uint8_t x, uint8_t y) {
	if (y != m.d8(g::cachedRow)) {
		m.d8(g::cachedRow) = y;
		uint16_t offset;
		// two cells per byte: the offset of a row is y * width / 2
		if (rowShift(m.d8(g::mapWidth), 2, y, offset)) m.setD16(g::rowPtr, offset);
	}
	const uint8_t byte = m.d8(uint16_t(m.d16(g::rowPtr) + (x >> 1)));
	return (x & 1) ? uint8_t(byte & 0x0F) : uint8_t(byte >> 4);
}

void selectActionRow(Machine &m, uint8_t y) {
	tileClassAt(m, m.d8(g::mapWidth), m.d8(g::mapHeight));   // positions rowPtr just behind the class nibbles
	uint16_t offset;
	// one action byte per cell: y * width
	if (rowShift(m.d8(g::mapWidth), 1, y, offset)) m.setD16(g::actionRowPtr, uint16_t(offset + m.d16(g::rowPtr)));
}

uint8_t actionAt(Machine &m, uint8_t x, uint8_t y) {
	selectActionRow(m, y);
	return m.d8(uint16_t(m.d16(g::actionRowPtr) + x));
}

Cell cellAt(Machine &m, uint8_t x, uint8_t y) {
	const uint8_t cls = tileClassAt(m, x, y);
	return {cls, actionAt(m, x, y)};
}

void setTileClass(Machine &m, uint8_t x, uint8_t y, uint8_t tileClass) {
	tileClassAt(m, x, y);   // selects the row
	const uint16_t offset = uint16_t(m.d16(g::rowPtr) + (x >> 1));
	m.d8(offset) = (x & 1) ? uint8_t((m.d8(offset) & 0xF0) | tileClass) : uint8_t((m.d8(offset) & 0x0F) | (tileClass << 4));
}

bool applyCellChange(Machine &m, uint8_t x, uint8_t y) {
	const uint8_t cls = m.d8(g::newTileClass);
	if (cls & 0x80) return false;
	setTileClass(m, x, y, cls);
	selectActionRow(m, y);
	m.d8(uint16_t(m.d16(g::actionRowPtr) + x)) = m.d8(g::newTileAction);
	return true;
}

bool changeCellFromLocation(Machine &m, uint8_t index, uint8_t x, uint8_t y) {
	const uint16_t record = m.d16(g::locationDetailsPtr);
	const uint8_t first = m.d8(uint16_t(record + index));
	if (first == 0xFE || first == 0xFD) {   // put back what an earlier change replaced
		m.d8(g::newTileClass) = m.d8(g::rememberedClass);
		m.d8(g::newTileAction) = m.d8(g::rememberedAction);
	} else {
		m.d8(g::newTileClass) = first;
		m.d8(g::newTileAction) = m.d8(uint16_t(record + uint8_t(index + 1)));
	}
	const Cell old = cellAt(m, x, y);
	m.d8(g::rememberedClass) = old.tileClass;
	m.d8(g::rememberedAction) = old.action;
	const bool changed = applyCellChange(m, x, y);
	return first == 0xFD ? false : changed;
}

void selectLocationRecord(Machine &m, uint8_t table, uint8_t index) {
	const uint16_t pointerTable = uint16_t(m.d16(uint16_t(g::locationRecOffsets + uint8_t(table << 1))) + m.d16(g::locationsPtr));
	m.setD16(g::locationDetailsPtr, m.d16(uint16_t(m.d16(pointerTable) + uint8_t(index << 1))));
}

XY partyMapPosition(Machine &m) {
	return {uint8_t(m.d8(g::saved2) + m.d8(g::viewOffsetX)), uint8_t(m.d8(uint16_t(g::saved2 + 1)) + m.d8(g::viewOffsetY))};
}

XY toViewPosition(Machine &m, uint8_t x, uint8_t y) {
	return {uint8_t(x - m.d8(g::viewOffsetX)), uint8_t(y - m.d8(g::viewOffsetY))};
}

void storeViewPosition(Machine &m) {
	const XY p = partyMapPosition(m);
	m.d8(g::viewX) = p.x;
	m.d8(g::viewY) = p.y;
}

void drawMapCell(Machine &m, uint8_t x, uint8_t y) {
	m.d8(0xBA04) = x;
	m.d8(0xBA05) = y;
	uint8_t al = m.d8(uint16_t(m.d16(g::locationsPtr) + 0x33));
	enum { Tile, Object } kind = Tile;
	bool draw = true;
	if (outsideMap(m, x, y)) {
		kind = Tile;   // draws the default tile al
	} else {
		const Cell c{tileClassAt(m, x, y), 0};
		auto record = [&]() { selectLocationRecord(m, c.tileClass, actionAt(m, x, y)); };
		bool tile = false;
		switch (c.tileClass) {
		case 5:
			al = 5;
			kind = Object;
			break;
		case 4:
			record();
			al = uint8_t(m.d8(uint16_t(m.d16(g::locationDetailsPtr) + 1)) & 0x7F);
			kind = al < 0x0A ? Object : Tile;
			if (kind == Tile) draw = true;
			break;
		case 9: {
			record();
			const uint8_t distance = cellDistance(m, x, y);
			if (distance < m.d8(0x46EE)) m.d8(0x46EE) = distance;
			al = distance;
			const uint8_t dayTime = m.d8(uint16_t(g::saved2 + 0x0C));
			if (dayTime < 6 || dayTime >= 0x12) { al = 8; kind = Object; } else tile = true;
			break;
		}
		default:
			tile = true;
			break;
		}
		if (tile) {
			setViewRowPointer(m, y);
			al = m.d8(uint16_t(m.d16(0x46CA) + x));
			kind = Tile;
		}
	}
	(void)draw;
	if (kind == Object) {
		setViewRowPointer(m, y);
		const uint8_t ah = m.d8(uint16_t(m.d16(0x46CA) + x));
		uint8_t sx = x, sy = y;
		if (cellToScreen(m, sx, sy)) return;
		m.d8(0x4685) = sx;
		m.d8(0x4686) = sy;
		X86Regs r;
		r.al = al; r.ah = ah; r.dl = sx; r.bl = sy;
		legacy::call(m, 0x000C, r);   // j_start_2
	} else {
		uint8_t sx = x, sy = y;
		if (cellToScreen(m, sx, sy)) return;
		m.d8(0x4685) = sx;
		m.d8(0x4686) = sy;
		X86Regs r;
		r.al = al; r.dl = sx; r.bl = sy;
		legacy::call(m, 0x0009, r);   // j_start_1
	}
}

} // namespace wl

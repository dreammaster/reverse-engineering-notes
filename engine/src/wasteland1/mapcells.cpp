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

void redrawViewColumn(Machine &m) {
	const uint8_t x = uint8_t((m.d8(0x4685) >> 1) + m.d8(g::saved2));
	uint8_t y = m.d8(uint16_t(g::saved2 + 1));
	m.d8(0xAA44) = x;
	m.d8(0xAA43) = y;
	for (int i = 9; i != 0; --i) {
		m.d8(0xAA31) = uint8_t(i);
		drawMapCell(m, x, y);
		y = uint8_t(y + 1);
		m.d8(0xAA43) = y;
	}
}

void redrawViewRow(Machine &m) {
	const uint8_t y = uint8_t((m.d8(0x4686) >> 4) + m.d8(uint16_t(g::saved2 + 1)));
	uint8_t x = m.d8(g::saved2);
	m.d8(0xAA42) = y;
	m.d8(0xAA41) = x;
	for (int i = m.d8(0x46A3) ? m.d8(0x46A3) : 256; i != 0; --i) {
		m.d8(0xAA30) = uint8_t(i);
		drawMapCell(m, x, y);
		x = uint8_t(x + 1);
		m.d8(0xAA41) = x;
	}
}

void scrollViewUp(Machine &m) {
	--m.d8(uint16_t(g::saved2 + 1));
	if (m.d8(0x4724) != 0) return;
	X86Regs r;
	legacy::call(m, 0x002A, r);   // j_start_15
	m.d8(0x4686) = 0x10;
	redrawViewRow(m);
	m.d8(0x4686) = 0;
	redrawViewRow(m);
}

void scrollViewLeft(Machine &m) {
	--m.d8(g::saved2);
	if (m.d8(0x4724) != 0) return;
	X86Regs r;
	legacy::call(m, 0x0024, r);   // j_start_16
	m.d8(0x4685) = 2;
	redrawViewColumn(m);
	m.d8(0x4685) = 0;
	redrawViewColumn(m);
}

void scrollViewRight(Machine &m) {
	++m.d8(g::saved2);
	if (m.d8(0x4724) != 0) return;
	X86Regs r;
	legacy::call(m, 0x0021, r);   // j_start_17
	m.d8(0x4685) = 0x22;
	redrawViewColumn(m);
	m.d8(0x4685) = 0x24;
	redrawViewColumn(m);
}

void scrollViewDown(Machine &m) {
	++m.d8(uint16_t(g::saved2 + 1));
	if (m.d8(0x4724) != 0) return;
	X86Regs r;
	legacy::call(m, 0x0027, r);   // j_start_18
	m.d8(0x4686) = 0x70;
	redrawViewRow(m);
	m.d8(0x4686) = 0x80;
	redrawViewRow(m);
}

void redrawViewObjects(Machine &m) {
	m.d8(0x46EE) = 0xFF;
	uint8_t y = m.d8(uint16_t(g::saved2 + 1));
	for (int row = 9; row != 0; --row, y = uint8_t(y + 1)) {
		m.d8(0xAA32) = uint8_t(row);
		m.d8(0xAA3E) = y;
		if (!(y & 0x80)) {
			if (y >= m.d8(0x46A4)) return;
			m.d8(0xAA33) = m.d8(0x46A3);
			uint8_t x = m.d8(g::saved2);
			for (int col = m.d8(0x46A3) ? m.d8(0x46A3) : 256; col != 0; --col, x = uint8_t(x + 1)) {
				m.d8(0xAA33) = uint8_t(col);
				m.d8(0xAA3F) = x;
				if (x & 0x80) continue;
				if (x >= m.d8(0x46A5)) break;
				const uint8_t cls = tileClassAt(m, x, y);
				if (cls == 5 || cls == 4 || cls == 9) drawMapCell(m, x, y);
			}
		}
	}
}

void partyCell(Machine &m, uint8_t &x, uint8_t &y) {
	x = uint8_t(m.d8(g::saved2) + m.d8(0x46A8));
	y = uint8_t(m.d8(uint16_t(g::saved2 + 1)) + m.d8(0x46A9));
}

uint8_t selectCellLocation(Machine &m, uint8_t x, uint8_t y) {
	const uint8_t cls = tileClassAt(m, x, y);
	selectLocationRecord(m, cls, actionAt(m, x, y));
	return cls;
}

void drawOtherParties(Machine &m) {
	const uint8_t active = m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup));
	for (int group = m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups)); group >= 0; --group) {
		m.d8(0xAA4C) = uint8_t(group);
		if (group == active) continue;
		setPartyIndexesPtr(m, uint8_t(group));
		const uint16_t indexes = m.d16(g::partyIndexesPtr);
		if (m.d8(uint16_t(indexes + 0x0A)) != m.d8(uint16_t(g::saved2 + g::s2::field7))) continue;
		const uint8_t x = m.d8(uint16_t(indexes + 8)), y = m.d8(uint16_t(indexes + 9));
		if (outsideView(m, x, y)) continue;
		setViewRowPointer(m, y);
		const uint8_t tile = m.d8(uint16_t(m.d16(0x46CA) + x));
		uint8_t sx = x, sy = y;
		cellToScreen(m, sx, sy);
		if (m.d8(0x46B9) != 0 && sy >= 0x0D) continue;
		m.d8(0x4685) = sx;
		m.d8(0x4686) = sy;
		X86Regs r;
		r.al = 9; r.ah = tile; r.dl = sx; r.bl = sy;
		legacy::call(m, 0x000C, r);   // j_start_2
		legacy::call(m, 0x9E30, r);   // clearInverseText
	}
	setPartyIndexesPtr(m, active);
}

void drawPartyMarker(Machine &m) {
	uint8_t x, y;
	partyCell(m, x, y);
	m.d8(0x46A6) = x;
	m.d8(0x46A7) = y;
	uint8_t tile;
	if (tileClassAt(m, x, y) == 4) {
		partyCell(m, x, y);
		selectCellLocation(m, x, y);
		tile = uint8_t(m.d8(uint16_t(m.d16(g::locationDetailsPtr) + 1)) & 0x7F);
	} else {
		setViewRowPointer(m, m.d8(0x46A7));
		tile = m.d8(uint16_t(m.d16(0x46CA) + m.d8(0x46A6)));
	}
	uint8_t sx = m.d8(0x46A6), sy = m.d8(0x46A7);
	cellToScreen(m, sx, sy);
	m.d8(0x4685) = sx;
	m.d8(0x4686) = sy;
	X86Regs r;
	r.al = 7; r.ah = tile; r.dl = sx; r.bl = sy;
	legacy::call(m, 0x000C, r);   // j_start_2
}

void redrawViewAll(Machine &m) {
	uint8_t x, y;
	partyCell(m, x, y);
	m.d8(0x46A6) = x;
	m.d8(0x46A7) = y;
	redrawViewObjects(m);
	drawOtherParties(m);
	drawPartyMarker(m);
}

void drawStatusBar(Machine &m) {
	auto put = [&](uint8_t ch) { X86Regs r; r.al = ch; legacy::call(m, 0x0039, r); };   // j_writeColorCharacter
	auto number = [&](uint8_t value) {
		X86Regs r; r.al = value; legacy::call(m, 0x76A8, r);   // setNumberValueShort
		const uint8_t bcd = m.d8(0x466E);
		put(uint8_t(bcd >> 4));
		put(bcd & 0x0F);
	};
	auto X = [&]() -> uint8_t & { return m.d8(g::stringXPos); };
	auto Y = [&]() -> uint8_t & { return m.d8(g::stringYPos); };
	const bool plain = m.d8(0x8DD9) == 0;
	if (plain) {
		X() = 0x1C; Y() = 0;
		put(0x0A);
		number(m.d8(uint16_t(g::saved2 + 0x0C)));
		put(0x0C);
		number(m.d8(uint16_t(g::saved2 + 0x0B)));
		put(0x0B);
		m.d8(0x4707) = 0;
	}
	uint8_t glyph = 0x7E;
	for (uint8_t member = 1;; ++member) {
		setCurrentCharacter(m, member);
		if (findItemById(m, 0x30) >= 0) break;
		if (member >= m.d8(uint16_t(g::saved2 + g::s2::membersInGroup))) {
			m.d8(0x46EE) = 0xFF;
			glyph = 0x7A;
			break;
		}
	}
	X() = 0x26; Y() = 0x10;
	put(glyph++); put(glyph++);
	X() = 0x26; --Y();
	put(glyph++); put(glyph++);
	X() = 0x26; --Y();
	uint8_t half = 0;
	const uint8_t level = uint8_t(m.d8(0x46EE) >> 3);
	if (level < 9) {
		const uint8_t rest = uint8_t(9 - level);
		half = rest & 1;
		uint8_t full = uint8_t(rest >> 1);
		while (full != 0) {
			if (Y() > 0x0C) { put(0x8A); put(0x8B); } else { put(0x86); put(0x87); }
			if (m.d8(0x8DD9) != 0) return;
			X() = 0x26; --Y();
			--full;
		}
		if (Y() != 0x0A) {
			if (half != 0) {
				if (Y() > 0x0C) { put(0x88); put(0x89); } else { put(0x84); put(0x85); }
				half = 0;
				if (m.d8(0x8DD9) != 0) return;
				X() = 0x26; --Y();
			}
		}
	}
	if (!(level < 9 && Y() == 0x0A)) {
		for (;;) {   // 17F66
			if (Y() == 0x0A) break;
			put(0x82); put(0x83);
			if (m.d8(0x8DD9) != 0) return;
			X() = 0x26;
			if (--Y() == 0) break;
		}
	}
	glyph = half != 0 ? 0x76 : 0x72;
	put(glyph++); put(glyph++);
	X() = 0x26; --Y();
	put(glyph++); put(glyph);
}

void drawPartyCell(Machine &m) {
	X86Regs r;
	legacy::call(m, 0x9E30, r);   // clearInverseText
	drawMapCell(m, m.d8(0x46A6), m.d8(0x46A7));
}

} // namespace wl

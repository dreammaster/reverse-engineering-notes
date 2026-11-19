#include "wasteland1/mouse.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/text.h"
#include <algorithm>
#include <cstdlib>

namespace wl {

namespace {
constexpr uint16_t buttonMask1 = 0x7DF3, buttonMask2 = 0x7DF5, buttonState = 0x7DF7, mouseXVar = 0x7DF9, mouseYVar = 0x7DFB;
constexpr uint16_t lastMask1 = 0xCA65, lastMask2 = 0xCA67, scanMask = 0xCA69, selected = 0x8DD6, cursorShown = 0x8DCB, cursorNum = 0x8DCD;
constexpr uint16_t hotspotTable = 0xCAEB, hotspotEnd = 0xCBBD, hotspotSize = 10;

bool bitSet(Machine &m, uint8_t bit) {   // bit of the mask pair (ds:7DF3 / 7DF5)
	return bit < 16 ? (m.d16(buttonMask1) >> bit) & 1 : (m.d16(buttonMask2) >> (bit - 16)) & 1;
}
void drawCursor(Machine &m) {   // 10042 (j_start_10): redraws the mouse cursor
	X86Regs x;
	legacy::call(m, 0x0042, x);
}
void releaseSelected(Machine &m) { releaseHotspot(m); }   // 18B60: un-highlights the selected button
}

bool mouseChanged(Machine &m) {
	const Input &in = m.input;
	const uint16_t cx = uint16_t(std::clamp(in.mouseX, 0, in.rangeMaxX)), dx = uint16_t(std::clamp(in.mouseY, 0, in.rangeMaxY));
	const uint8_t bl = in.buttons;
	const uint16_t mask1 = m.d16(buttonMask1), mask2 = m.d16(buttonMask2);
	if (cx == m.d16(mouseXVar) && dx == m.d16(mouseYVar) && bl == m.d8(buttonState) && mask2 == m.d16(lastMask2) && mask1 == m.d16(lastMask1) &&
	    m.d8(buttonState) == 0 && m.d16(cursorShown) != 0)
		return false;
	m.setD16(lastMask1, mask1);
	m.setD16(lastMask2, mask2);
	m.setD16(mouseXVar, cx);
	m.setD16(mouseYVar, dx);
	uint8_t bh = 0;
	const uint8_t changed = uint8_t(m.d8(buttonState) ^ bl);
	if (changed != 0) {   // bits 4..7 of the high byte: buttons that went down since the last poll
		const uint8_t al = uint8_t((changed << 4) | 0x0F), ah = uint8_t(changed | 0xF0);
		bh = uint8_t((uint8_t(bl << 4) | m.d8(buttonState)) & al & ah);
	}
	m.setD16(buttonState, uint16_t(bl | (bh << 8)));
	return true;
}

bool pollMouse(Machine &m, uint8_t &al) {
	if (std::getenv("WL_MOUSE_LEGACY")) {   // reference run: the recompiled original
		X86Regs x;
		legacy::call(m, 0x8860, x);
		al = x.al;
		return x.cf;
	}
	if (!mouseChanged(m)) return false;
	const uint16_t cx = m.d16(mouseXVar), dx = m.d16(mouseYVar);
	m.setD16(scanMask, m.d16(buttonMask1));
	uint16_t bit = 1;
	uint8_t index = 0;
	for (uint16_t row = hotspotTable; row != hotspotEnd; row = uint16_t(row + hotspotSize)) {
		const bool active = (m.d16(scanMask) & bit) != 0;
		if (active && cx >= m.d16(row) && dx >= m.d16(uint16_t(row + 2)) && cx <= m.d16(uint16_t(row + 4)) && dx <= m.d16(uint16_t(row + 6))) {
			const uint8_t sel = m.d8(selected);
			if (sel != 0 && index != sel && bitSet(m, sel)) releaseSelected(m);
			X86Regs x;
			x.bl = index;
			x.setCx(cx);
			x.setDx(dx);
			legacy::call(m, m.d16(uint16_t(row + 8)), x);
			X86Regs draw;
			drawCursor(m);
			al = x.al;
			return x.cf;
		}
		bit = uint16_t(bit << 1);
		if (++index == 16) {
			m.setD16(scanMask, m.d16(buttonMask2));
			bit = 1;
		}
	}
	const uint8_t sel = m.d8(selected);
	if (sel != 0 && bitSet(m, sel)) releaseSelected(m);
	m.setD16(cursorNum, 0);
	drawCursor(m);
	return false;
}


namespace {
constexpr uint16_t cursorNumVar = 0x8DCD, selectedVar = 0x8DD6, stateVar = 0x7DF7, mouseX = 0x7DF9, mouseY = 0x7DFB;
bool leftPressed(Machine &m) { return (m.d16(stateVar) & 0x1000) != 0; }   // the left button went down since the last poll
}

HotspotResult hotspotButton(Machine &m, uint8_t index) {   // 18BFF: a button of the bottom row; al = its key
	m.setD16(cursorNumVar, 1);
	if (index != m.d8(selectedVar)) selectHotspot(m, index);   // 18B4C
	if (!leftPressed(m)) return {false, 0};
	return {true, m.d8(uint16_t(0xCBBD + uint16_t(index - 4) * 6 + 3))};
}

HotspotResult hotspotRoster(Machine &m) {   // 18C2E: the roster rows; a click answers the digit of the member
	m.setD16(cursorNumVar, 0);
	const int row = int(m.d16(mouseY)) - 0x7D;
	if (row < 0) return {false, 0};
	const uint8_t member = uint8_t((row >> 3) + 1);
	if (member > m.d8(0x4653)) return {false, 0};
	m.setD16(cursorNumVar, 1);
	if (!leftPressed(m)) return {false, 0};
	return {true, uint8_t(member + '0')};
}

HotspotResult hotspotMap(Machine &m) {   // 18C62: the map view; the cursor is an arrow towards the pointer, a click is a direction key
	const uint16_t cx = m.d16(mouseX), dx = m.d16(mouseY);
	const bool clicked = (m.d16(stateVar) & 1) != 0;
	if (dx >= 0x38 && cx >= 0x88 && int16_t(dx) < 0x48 && int16_t(cx) < 0x98) {   // the centre: wait / look
		m.setD16(cursorNumVar, 6);
		if (!clicked) return {false, 0};
		return {true, 0x1B};
	}
	uint16_t bx;
	if (cx < uint16_t(dx + 0x50)) bx = cx < uint16_t(0xE0 - dx) ? 4 : 3;   // the two diagonals divide the view into four sectors
	else bx = cx < uint16_t(0xD0 - dx) ? 2 : 5;
	m.setD16(cursorNumVar, bx);
	if (!clicked) return {false, 0};
	uint8_t al = m.d8(uint16_t(0xC05D + uint16_t(bx - 2)));
	if (m.d8(0x8DDA) != 0 && al == 0x1B) al = 0x20;
	return {true, al};
}

HotspotResult hotspotList(Machine &m) {   // 18CE7: the list rows of a window (one byte per text row at ds:8DDC)
	m.setD16(cursorNumVar, 0);
	const uint16_t row = uint16_t((m.d16(mouseY) - 5) >> 3);
	if (m.d8(uint16_t(0x8DDC + row)) == 0) return {false, 0};
	m.setD16(cursorNumVar, 1);
	if (!leftPressed(m)) return {false, 0};
	return {true, m.d8(uint16_t(0x8DDC + row))};
}

HotspotResult hotspotInfo(Machine &m) {   // 18D16: runs 174B7 (the info window) and answers RETURN on a click
	X86Regs x;
	legacy::call(m, 0x74B7, x);
	if (!leftPressed(m)) return {false, 0};
	return {true, 0x0D};
}

} // namespace wl

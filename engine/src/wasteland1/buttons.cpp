#include "wasteland1/buttons.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"

namespace wl {

namespace {
constexpr uint16_t buttonSelected = 0x4688, buttonLast = 0x4689, buttonRow = 0x468A, buttonPos = 0x468B, buttonTexts = 0x468D,
                   buttonHandlers = 0x468F, colorX = 0x8DD0, colorY = 0x8DD1, rowStart = 0x8DD7, highlightFlag = 0x722F;

void putc(Machine &m, uint8_t glyph) {   // j_writeColorCharacter (10039)
	X86Regs x;
	x.al = glyph;
	legacy::call(m, 0x0039, x);
}
uint16_t textLength(Machine &m) {
	const uint16_t text = m.d16(g::textString);
	uint16_t n = 0;
	while (m.d8(uint16_t(text + n)) != 0 && n != 0xFFFF) ++n;
	return n;
}
uint8_t glyphOf(uint8_t c) { return c == ' ' ? 49 : uint8_t((c & 0xDF) + 0xD7); }
}

void getButtonsText(Machine &m) {
	m.setD16(g::textString, m.d16(uint16_t(m.d16(buttonTexts) + m.d8(buttonRow) * 2)));
}

bool findButton(Machine &m) {
	getButtonsText(m);
	const uint16_t text = m.d16(g::textString);
	uint8_t dl = 0, bl = 0;
	while (dl != m.d8(buttonSelected)) {
		const uint8_t c = m.d8(uint16_t(text + bl));
		if (c == 0) { m.d8(buttonPos) = bl; return false; }
		if (c != ' ') {
			if (++bl != 0) continue;
		}
		++dl;
		if (++bl == 0) break;
	}
	m.d8(buttonPos) = bl;
	return true;
}

void drawButton(Machine &m, bool highlight) {
	m.d8(highlightFlag) = highlight ? 0xFF : 0;
	if (!findButton(m)) { m.d8(highlightFlag) = 0; return; }
	const uint8_t position = m.d8(buttonPos);
	const uint8_t words = uint8_t((textLength(m) + 1) >> 1);
	m.setD16(g::textString, uint16_t(m.d16(g::textString) + position));
	m.d8(colorX) = uint8_t(20 - words + position);
	m.d8(colorY) = 24;
	for (;;) {
		const uint8_t c = m.d8(m.d16(g::textString));
		if (c == 0) { putc(m, 0x28); m.d8(highlightFlag) = 0; return; }
		if (c == ' ') { putc(m, 0x31); m.d8(highlightFlag) = 0; return; }
		putc(m, glyphOf(c));
		m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
	}
}

void drawButtonsRow(Machine &m) {
	getButtonsText(m);
	const uint8_t words = uint8_t((textLength(m) + 1) >> 1);
	const uint8_t start = uint8_t(19 - words);
	m.d8(rowStart) = start;
	m.d8(colorX) = 0;
	m.d8(colorY) = 24;
	uint8_t glyph = 0x5C;
	if (start != 0) {
		glyph = 16;
		do {
			putc(m, glyph);
			glyph = 18;
		} while (m.d8(rowStart) != m.d8(colorX));
		glyph = 23;
	}
	putc(m, glyph);
	for (;;) {
		const uint8_t c = m.d8(m.d16(g::textString));
		if (c == 0) break;
		putc(m, glyphOf(c));
		m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
		if (m.d16(g::textString) == 0) break;
	}
	putc(m, 40);
	while (m.d8(colorX) != 39) putc(m, 18);
	putc(m, 0x11);
	drawButton(m, true);
}

void buttonKey(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint8_t key = D(g::lastKey);
	if (key == 0) return;
	bool run = false;
	if (key == 0xF3) {   // left
		drawButton(m, false);
		uint8_t v = D(buttonSelected);
		if (v <= D(buttonLast)) {
			v = uint8_t(v - 1);
			if (v & 0x80) v = D(buttonLast);
		} else {
			v = D(buttonLast);
		}
		D(buttonSelected) = v;
		drawButton(m, true);
		return;
	}
	if (key == 0xF4) {   // right
		drawButton(m, false);
		D(buttonSelected) = uint8_t(D(buttonSelected) + 1);
		if (D(buttonSelected) > D(buttonLast)) D(buttonSelected) = 0;
		drawButton(m, true);
		return;
	}
	if (key == 0x0D) {
		if (D(buttonSelected) > D(buttonLast)) return;
		run = true;
	} else {
		const uint8_t saved = D(buttonSelected);
		uint8_t found = 0xFF;
		for (uint8_t dl = 0;;) {   // a button whose first letter is the key
			D(buttonSelected) = dl;
			findButton(m);
			if (D(uint16_t(m.d16(g::textString) + D(buttonPos))) == key) { found = D(buttonSelected); break; }
			dl = uint8_t(D(buttonSelected) + 1);
			if (dl > D(buttonLast)) break;
		}
		if (found == 0xFF) {   // the key may also be a later letter of the button the cursor is on
			const uint8_t last = D(buttonSelected);
			D(buttonSelected) = 0xFF;
			findButton(m);
			D(buttonSelected) = last;
			D(buttonSelected) = uint8_t(D(buttonSelected) + 1);
			uint8_t bl = uint8_t(D(buttonPos) + 1);
			for (;;) {
				const uint8_t c = D(uint16_t(m.d16(g::textString) + bl));
				if (c == 0) break;
				if (c == key) { found = D(buttonSelected); break; }
				if (++D(buttonSelected) == 0) break;
				bl = uint8_t(bl + 1);
			}
			if (found == 0xFF) { D(buttonSelected) = saved; return; }
		}
		D(buttonSelected) = saved;
		drawButton(m, false);
		D(buttonSelected) = found;
		drawButton(m, true);
		run = true;
	}
	if (!run) return;
	const uint16_t table = m.d16(uint16_t(m.d16(buttonHandlers) + D(buttonRow) * 2));
	m.setD16(0x4661, table);
	const uint16_t handler = m.d16(uint16_t(table + D(buttonSelected) * 2));
	m.setD16(0xB250, handler);
	X86Regs x;
	legacy::call(m, handler, x);
	drawButtonsRow(m);
}

} // namespace wl
